#!/usr/bin/env node

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

import { closeSync, openSync, readSync, writeSync } from "node:fs";
import { resolve } from "node:path";
import { StringDecoder } from "node:string_decoder";
import { parseArgs } from "node:util";

import { Groonga, GRN_SUCCESS } from "../lib/libgroonga.mjs";

function parseArguments(rawArguments) {
  // parseArgs() accepts a short option only as an alias of a long
  // option. So "--new" is also accepted unlike the groonga command.
  const { values, positionals } = parseArgs({
    args: rawArguments,
    options: {
      new: { type: "boolean", short: "n" },
      "input-fd": { type: "string" },
      "output-fd": { type: "string" },
      file: { type: "string" },
      "log-path": { type: "string" },
      "query-log-path": { type: "string" },
      "working-directory": { type: "string" },
    },
    allowPositionals: true,
    strict: true,
  });
  const [databasePath = null, ...commands] = positionals;
  return {
    isNew: values.new ?? false,
    inputFD: Number(values["input-fd"] ?? process.stdin.fd),
    outputFD: Number(values["output-fd"] ?? process.stdout.fd),
    file: values.file ?? null,
    logPath: values["log-path"] ?? null,
    queryLogPath: values["query-log-path"] ?? null,
    workingDirectory: values["working-directory"] ?? null,
    databasePath,
    commands,
  };
}

function* readLines(fd) {
  const buffer = Buffer.alloc(65536);
  // A multibyte character may be split by the read boundary.
  // StringDecoder keeps an incomplete character but Buffer#toString()
  // replaces it with U+FFFD.
  const decoder = new StringDecoder("utf8");
  let rest = "";
  for (;;) {
    let nBytes;
    try {
      nBytes = readSync(fd, buffer, 0, buffer.length, null);
    } catch (error) {
      if (error.code === "EAGAIN") {
        continue;
      }
      if (error.code === "EOF") {
        nBytes = 0;
      } else {
        throw error;
      }
    }
    if (nBytes === 0) {
      break;
    }
    rest += decoder.write(buffer.subarray(0, nBytes));
    const lines = rest.split("\n");
    rest = lines.pop();
    for (const line of lines) {
      yield line;
    }
  }
  rest += decoder.end();
  if (rest.length > 0) {
    yield rest;
  }
}

// writeSync() may write only a part of the given bytes. This does the
// same thing as ensure_write() in src/groonga.c.
function writeAll(fd, bytes) {
  let rest = bytes;
  while (rest.length > 0) {
    let nWritten;
    try {
      nWritten = writeSync(fd, rest);
    } catch (error) {
      if (error.code === "EAGAIN") {
        continue;
      }
      throw error;
    }
    rest = rest.subarray(nWritten);
  }
}

let options;
try {
  options = parseArguments(process.argv.slice(2));
} catch (error) {
  writeSync(process.stderr.fd, `groonga.mjs: ${error.message}\n`);
  process.exit(1);
}
if (!options.databasePath) {
  writeSync(process.stderr.fd, "groonga.mjs: database path is missing\n");
  process.exit(1);
}
// groonga resolves a relative path by the current directory. So we
// must change the current directory before we open the database.
const workingDirectory = options.workingDirectory || process.cwd();
if (options.workingDirectory) {
  process.chdir(options.workingDirectory);
}

const groonga = await Groonga.open({
  logPath: options.logPath && resolve(workingDirectory, options.logPath),
  queryLogPath:
    options.queryLogPath && resolve(workingDirectory, options.queryLogPath),
});
const context = groonga.createContext();
const databasePath = resolve(workingDirectory, options.databasePath);
try {
  if (options.isNew) {
    context.createDatabase(databasePath);
  } else {
    context.openDatabase(databasePath);
  }
} catch (error) {
  writeSync(process.stderr.fd, `groonga.mjs: ${error.message}\n`);
  process.exit(1);
}

const execute = (command) => {
  const output = context.send(command);
  if (output.length > 0) {
    writeAll(options.outputFD, output);
  }
  // GRN_API_ENTER in grn_ctx_send() clears the error of the context.
  // So we don't need to clear it here.
  return !context.isQuitting;
};

if (options.commands.length > 0) {
  execute(options.commands.join(" "));
} else {
  let fd;
  try {
    fd = options.file ? openSync(options.file, "r") : options.inputFD;
  } catch (error) {
    writeSync(
      process.stderr.fd,
      `groonga.mjs: can't open input file: ${error.message}\n`,
    );
    process.exit(1);
  }
  try {
    for (const line of readLines(fd)) {
      if (!execute(line)) {
        break;
      }
    }
  } finally {
    if (options.file) {
      closeSync(fd);
    }
  }
}

const rc = context.rc;
groonga.close();
process.exit(rc === GRN_SUCCESS ? 0 : 1);
