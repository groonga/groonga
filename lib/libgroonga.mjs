// Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { WASI } from "node:wasi";

function resolveSibling(name) {
  return resolve(dirname(fileURLToPath(import.meta.url)), name);
}

// See include/groonga/groonga.h and include/groonga/output.h.
export const GRN_SUCCESS = 0;
const GRN_BULK = 0x02;
const GRN_DB_TEXT = 15;
const GRN_CTX_TAIL = 1 << 1;
const GRN_CONTENT_NONE = 0;
const GRN_CONTENT_GROONGA_COMMAND_LIST = 5;
const NEWLINE = 0x0a;
const NEWLINE_BYTES = new Uint8Array([NEWLINE]);

// Register a JavaScript function as a function pointer of the given
// WebAssembly instance and return the function pointer. `name` is the
// name of the function type in libgroonga-callback.wasm such as
// "grn_recv_handler_func". See lib/wasm_callback.c for why we need
// libgroonga-callback.wasm.
async function createFunctionPointer(exports, name, callback) {
  const path = resolveSibling("libgroonga-callback.wasm");
  const callbackModule = await WebAssembly.compile(await readFile(path));
  const { exports: callbackExports } = await WebAssembly.instantiate(
    callbackModule,
    { env: { [name]: callback } },
  );
  const table = exports.__indirect_function_table;
  const pointer = table.grow(1);
  table.set(pointer, callbackExports[name]);
  return pointer;
}

// libgroonga.wasm. One Groonga has one libgroonga.wasm instance and
// its contexts. Use Groonga.open() to create one.
export class Groonga {
  // Load libgroonga.wasm and initialize it. `logPath` and
  // `queryLogPath` must be set before grn_init(). So they are options
  // of this instead of setters.
  static async open({ logPath = null, queryLogPath = null } = {}) {
    const wasi = new WASI({
      version: "preview1",
      args: ["groonga"],
      env: process.env,
      preopens: { "/": "/" },
      returnOnExit: true,
    });
    const module = await WebAssembly.compile(
      await readFile(resolveSibling("libgroonga.wasm")),
    );
    const instance = await WebAssembly.instantiate(
      module,
      wasi.getImportObject(),
    );
    wasi.initialize(instance);
    const groonga = new Groonga(instance.exports);
    if (logPath) {
      groonga.withCString(logPath, (pointer) => {
        groonga.exports.grn_default_logger_set_path(pointer);
      });
    }
    if (queryLogPath) {
      groonga.withCString(queryLogPath, (pointer) => {
        groonga.exports.grn_default_query_logger_set_path(pointer);
      });
    }
    const rc = groonga.exports.grn_init();
    if (rc !== GRN_SUCCESS) {
      throw new Error(
        `failed to initialize Groonga: ${groonga.rcToString(rc)}`,
      );
    }
    try {
      // All contexts share one function pointer. The handler
      // dispatches by its context argument.
      groonga.recvHandlerFunc = await createFunctionPointer(
        groonga.exports,
        "grn_recv_handler_func",
        (ctx, flags) => groonga.contexts.get(ctx).receiveOutput(flags),
      );
    } catch (error) {
      groonga.exports.grn_fin();
      throw error;
    }
    return groonga;
  }

  constructor(exports) {
    this.exports = exports;
    this.encoder = new TextEncoder();
    this.decoder = new TextDecoder();
    this.contexts = new Map();
    this.databases = new Set();
    this.recvHandlerFunc = 0;
  }

  createContext() {
    const context = new Context(this);
    this.contexts.set(context.ctx, context);
    return context;
  }

  removeContext(context) {
    this.contexts.delete(context.ctx);
  }

  addDatabase(database) {
    this.databases.add(database);
  }

  removeDatabase(database) {
    this.databases.delete(database);
  }

  close() {
    // A database must be closed before its context because
    // grn_obj_close() needs a context.
    for (const database of [...this.databases]) {
      database.close();
    }
    for (const context of [...this.contexts.values()]) {
      context.close();
    }
    const rc = this.exports.grn_fin();
    if (rc !== GRN_SUCCESS) {
      throw new Error(`failed to finalize Groonga: ${this.rcToString(rc)}`);
    }
  }

  [Symbol.dispose]() {
    this.close();
  }

  rcToString(rc) {
    return this.readCString(this.exports.grn_rc_to_string(rc));
  }

  // Copy the content in the WebAssembly memory. The content may be
  // invalidated by the next call of the C API. So we must copy it.
  readBytes(pointer, size) {
    return new Uint8Array(this.exports.memory.buffer, pointer, size).slice();
  }

  concatBytes(chunks) {
    if (chunks.length === 1) {
      return chunks[0];
    }
    let size = 0;
    for (const chunk of chunks) {
      size += chunk.length;
    }
    const bytes = new Uint8Array(size);
    let offset = 0;
    for (const chunk of chunks) {
      bytes.set(chunk, offset);
      offset += chunk.length;
    }
    return bytes;
  }

  decode(bytes) {
    return this.decoder.decode(bytes);
  }

  readCString(pointer) {
    const bytes = new Uint8Array(this.exports.memory.buffer, pointer);
    const end = bytes.indexOf(0);
    return this.decoder.decode(bytes.subarray(0, end < 0 ? 0 : end));
  }

  withCString(string, callback) {
    const bytes = this.encoder.encode(string);
    const pointer = this.exports.malloc(bytes.length + 1);
    try {
      const memory = new Uint8Array(this.exports.memory.buffer);
      memory.set(bytes, pointer);
      memory[pointer + bytes.length] = 0;
      return callback(pointer, bytes.length);
    } finally {
      this.exports.free(pointer);
    }
  }
}

// grn_ctx. Use Groonga.createContext() to create one.
export class Context {
  constructor(groonga) {
    this.groonga = groonga;
    this.exports = groonga.exports;
    this.ctx = this.exports.grn_ctx_open(0);
    this.database = null;
    this.output = [];
    this.outputError = null;
    this.isOutputting = false;
    this.receiveBuffer = 0;
    this.headBulk = 0;
    this.bodyBulk = 0;
    this.footBulk = 0;
    this.exports.grn_ctx_recv_handler_set(this.ctx, groonga.recvHandlerFunc, 0);
  }

  // Create a new database. It's used by this context.
  createDatabase(path) {
    return this.#newDatabase(path, (pointer) =>
      this.exports.grn_db_create(this.ctx, pointer, 0),
    );
  }

  // Open an existing database. It's used by this context.
  openDatabase(path) {
    return this.#newDatabase(path, (pointer) =>
      this.exports.grn_db_open(this.ctx, pointer),
    );
  }

  #newDatabase(path, open) {
    const db = this.groonga.withCString(path, open);
    if (db === 0) {
      throw new Error(
        `failed to open database: <${path}>: ${this.errorMessage}`,
      );
    }
    this.database = new Database(this, path, db);
    this.groonga.addDatabase(this.database);
    return this.database;
  }

  // Use the given database that is opened by another context.
  use(database) {
    this.exports.grn_ctx_use(this.ctx, database.db);
    this.database = database;
  }

  // Send one command and return its output as bytes. The output is
  // built by the recv handler. It's not a string because the output
  // type may be binary such as MessagePack.
  send(command) {
    this.output = [];
    this.outputError = null;
    this.groonga.withCString(command, (pointer, size) => {
      this.exports.grn_ctx_send(this.ctx, pointer, size, 0);
    });
    if (this.outputError) {
      throw this.outputError;
    }
    return this.groonga.concatBytes(this.output);
  }

  // Send one command and return its output as a string. Use
  // Context#send() for a binary output type.
  sendText(command) {
    return this.groonga.decode(this.send(command));
  }

  // grn_output_envelope() needs the command version and the current
  // expression of the running command. They are available only while
  // the command is running. So we must build the output in a recv
  // handler instead of after grn_ctx_send().
  //
  // This does the same thing as s_output() in src/groonga.c.
  receiveOutput(flags) {
    // This is called by libgroonga.wasm. An exception here unwinds
    // the WebAssembly frames of the running command and Groonga can't
    // clean up them. So we keep it and Context#send() raises it after
    // the command is finished.
    try {
      const isLast = (flags & GRN_CTX_TAIL) !== 0;
      const outputType = this.exports.grn_ctx_get_output_type(this.ctx);
      if (
        outputType === GRN_CONTENT_NONE ||
        outputType === GRN_CONTENT_GROONGA_COMMAND_LIST
      ) {
        this.receiveOutputRaw(isLast, outputType);
      } else {
        this.receiveOutputTyped(isLast);
      }
    } catch (error) {
      this.outputError ??= error;
    }
  }

  receiveOutputRaw(isLast, outputType) {
    const chunk = this.receive();
    if (chunk.size === 0) {
      return;
    }
    const bytes = this.groonga.readBytes(chunk.pointer, chunk.size);
    this.output.push(bytes);
    if (
      isLast &&
      outputType === GRN_CONTENT_GROONGA_COMMAND_LIST &&
      bytes[bytes.length - 1] !== NEWLINE
    ) {
      this.output.push(NEWLINE_BYTES);
    }
  }

  receiveOutputTyped(isLast) {
    // GRN_API_ENTER in the C API doesn't clear the return code here
    // because we are in an API call.
    const rc = this.rc;
    const chunk = this.receive();
    if (chunk.size === 0 && rc === GRN_SUCCESS) {
      return;
    }
    // grn_ctx_recv() rewinds the output buffer of the context. The
    // received content is valid until we write to the buffer again.
    // grn_bulk_rewind() in #prepareBulks() doesn't write to it.
    const { headBulk, bodyBulk, footBulk } = this.#prepareBulks();
    this.exports.grn_bulk_set(this.ctx, bodyBulk, chunk.pointer, chunk.size);
    if (this.isOutputting) {
      if (isLast) {
        this.exports.grn_output_envelope_close(this.ctx, footBulk, rc, 0, 0);
      }
    } else {
      if (isLast) {
        this.exports.grn_output_envelope(
          this.ctx,
          rc,
          headBulk,
          bodyBulk,
          footBulk,
          0,
          0,
        );
      } else {
        this.exports.grn_output_envelope_open(this.ctx, headBulk);
      }
      this.isOutputting = true;
    }
    let size = 0;
    for (const bulk of [headBulk, bodyBulk, footBulk]) {
      const bytes = this.#readBulk(bulk);
      if (bytes) {
        this.output.push(bytes);
        size += bytes.length;
      }
    }
    if (isLast) {
      if (size > 0) {
        this.output.push(NEWLINE_BYTES);
      }
      this.isOutputting = false;
    }
  }

  // The bulks for grn_output_envelope(). We create them once and
  // reuse them because the recv handler is called for each output.
  #prepareBulks() {
    if (this.headBulk === 0) {
      this.headBulk = this.#openBulk();
      this.bodyBulk = this.#openBulk();
      this.footBulk = this.#openBulk();
    } else {
      for (const bulk of [this.headBulk, this.bodyBulk, this.footBulk]) {
        this.exports.grn_bulk_rewind(this.ctx, bulk);
      }
    }
    return {
      headBulk: this.headBulk,
      bodyBulk: this.bodyBulk,
      footBulk: this.footBulk,
    };
  }

  receive() {
    // The output arguments of grn_ctx_recv(): char **content,
    // unsigned int *size and int *flags. We allocate them once and
    // reuse them because grn_ctx_recv() is called for each output.
    if (this.receiveBuffer === 0) {
      this.receiveBuffer = this.exports.malloc(4 * 3);
    }
    const contentPointer = this.receiveBuffer;
    const sizePointer = this.receiveBuffer + 4;
    const flagsPointer = this.receiveBuffer + 8;
    this.exports.grn_ctx_recv(
      this.ctx,
      contentPointer,
      sizePointer,
      flagsPointer,
    );
    const view = new DataView(this.exports.memory.buffer);
    return {
      pointer: view.getUint32(contentPointer, true),
      size: view.getUint32(sizePointer, true),
    };
  }

  #openBulk() {
    return this.exports.grn_obj_open(this.ctx, GRN_BULK, 0, GRN_DB_TEXT);
  }

  #readBulk(bulk) {
    const size = this.exports.grn_bulk_get_size(this.ctx, bulk);
    if (size === 0) {
      return null;
    }
    return this.groonga.readBytes(
      this.exports.grn_bulk_get_head(this.ctx, bulk),
      size,
    );
  }

  get rc() {
    return this.exports.grn_ctx_get_rc(this.ctx);
  }

  get errorMessage() {
    return this.groonga.readCString(
      this.exports.grn_ctx_get_error_message(this.ctx),
    );
  }

  get isQuitting() {
    return this.exports.grn_ctx_is_quitting(this.ctx) !== 0;
  }

  [Symbol.dispose]() {
    this.close();
  }

  // This doesn't close the database of this context because it may be
  // used by another context. Use Database#close() or Groonga#close()
  // to close it.
  close() {
    if (this.receiveBuffer !== 0) {
      this.exports.free(this.receiveBuffer);
      this.receiveBuffer = 0;
    }
    if (this.headBulk !== 0) {
      for (const bulk of [this.headBulk, this.bodyBulk, this.footBulk]) {
        this.exports.grn_obj_close(this.ctx, bulk);
      }
      this.headBulk = 0;
      this.bodyBulk = 0;
      this.footBulk = 0;
    }
    this.database = null;
    this.groonga.removeContext(this);
    this.exports.grn_ctx_close(this.ctx);
  }
}

// grn_obj for a database. Use Context#createDatabase() or
// Context#openDatabase() to create one.
export class Database {
  constructor(context, path, db) {
    this.context = context;
    this.path = path;
    this.db = db;
  }

  [Symbol.dispose]() {
    this.close();
  }

  // The context that opened this database must not be closed before
  // this because grn_obj_close() needs a context.
  close() {
    if (this.db === 0) {
      return;
    }
    this.context.groonga.removeDatabase(this);
    this.context.exports.grn_obj_close(this.context.ctx, this.db);
    this.db = 0;
  }
}
