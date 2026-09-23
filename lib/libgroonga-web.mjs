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

import { Groonga } from "./libgroonga.mjs";
import { MemoryFileSystem, WASI } from "./libgroonga-wasi.mjs";

export { Context, Database, Groonga, GRN_SUCCESS } from "./libgroonga.mjs";
export { MemoryFileSystem } from "./libgroonga-wasi.mjs";

async function compileSibling(name) {
  const url = new URL(name, import.meta.url);
  return WebAssembly.compileStreaming(fetch(url));
}

// A file system that downloads a file of a Groonga database when it's
// used at first. `baseURL` is the URL of the directory that has the
// database. `path` is the directory in this file system.
//
// A Groonga database consists of multiple files and there is no way
// to list them by HTTP. So we can't download them in advance without
// a list. We download a file on demand instead.
export class HTTPFileSystem extends MemoryFileSystem {
  constructor(baseURL, path = "/db") {
    super();
    this.baseURL = baseURL;
    this.path = path;
    this.missingNames = new Set();
  }

  lookup(path) {
    const node = super.lookup(path);
    if (node !== null) {
      return node;
    }
    const name = this.databaseFileName(path);
    if (name === null || this.missingNames.has(name)) {
      return null;
    }
    const content = fetchSync(new URL(name, this.baseURL));
    if (content === null) {
      // Groonga creates a new file for a new column and so on.
      this.missingNames.add(name);
      return null;
    }
    return this.addFile(path, content);
  }

  databaseFileName(path) {
    const prefix = `${this.path}/`;
    if (!path.startsWith(prefix)) {
      return null;
    }
    const name = path.slice(prefix.length);
    if (name === "" || name.includes("/")) {
      return null;
    }
    return name;
  }
}

// Download a file synchronously. WASI is synchronous. So we can't use
// fetch() that is asynchronous.
//
// XMLHttpRequest#responseType can be used only in a Worker when
// XMLHttpRequest is synchronous. We use the "x-user-defined" charset
// that maps each byte to a character in the main thread.
function fetchSync(url) {
  const request = new XMLHttpRequest();
  request.open("GET", url, false);
  let useArrayBuffer = true;
  try {
    request.responseType = "arraybuffer";
  } catch {
    useArrayBuffer = false;
    request.overrideMimeType("text/plain; charset=x-user-defined");
  }
  try {
    request.send();
  } catch {
    // The caller uses this in a WASI function. An exception here
    // unwinds the WebAssembly frames and Groonga can't clean up them.
    return null;
  }
  if (request.status !== 200) {
    return null;
  }
  if (useArrayBuffer) {
    return new Uint8Array(request.response);
  }
  const text = request.responseText;
  const content = new Uint8Array(text.length);
  for (let i = 0; i < text.length; i++) {
    content[i] = text.charCodeAt(i) & 0xff;
  }
  return content;
}

// Only an in-memory file system is available. Use HTTPFileSystem to
// use an existing database.
export async function open(options = {}) {
  const fileSystem = options.fileSystem ?? new MemoryFileSystem();
  const wasi = new WASI({ args: ["groonga"], env: {}, fileSystem });
  // Groonga writes a log to the standard error and so on. We can't
  // see it without this.
  wasi.onStandardOutput = (text) => console.log(text);
  wasi.onStandardError = (text) => console.error(text);
  const groonga = await Groonga.open({
    ...options,
    wasi,
    module: await compileSibling("libgroonga.wasm"),
    callbackModule: await compileSibling("libgroonga-callback.wasm"),
  });
  groonga.fileSystem = fileSystem;
  return groonga;
}
