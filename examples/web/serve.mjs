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

// A HTTP server for index.html. A Web browser can't load a module by
// file:// because of CORS. So we need a HTTP server.
//
// This directory is served as the root. /lib/ is served from the
// installed Groonga for WASI because libgroonga.wasm is built by the
// build system.
//
// Usage: node serve.mjs [--prefix PREFIX] [--port PORT]

import { createReadStream, statSync } from "node:fs";
import { createServer } from "node:http";
import { extname, join, normalize } from "node:path";
import { fileURLToPath } from "node:url";
import { parseArgs } from "node:util";

const { values } = parseArgs({
  options: {
    prefix: { type: "string", default: "/tmp/local" },
    port: { type: "string", default: "8080" },
  },
});
const example = fileURLToPath(new URL("./", import.meta.url));
const prefix = normalize(values.prefix + "/");
const port = Number(values.port);

const contentTypes = {
  ".html": "text/html; charset=utf-8",
  ".mjs": "text/javascript; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".wasm": "application/wasm",
};

function resolveLocalPath(path) {
  // libgroonga.wasm and libgroonga.mjs are installed into PREFIX/lib/.
  const root = path.startsWith("/lib/") ? prefix : example;
  const localPath = join(root, normalize(path));
  return localPath.startsWith(root) ? localPath : null;
}

createServer((request, response) => {
  let path = decodeURIComponent(
    new URL(request.url, "http://localhost").pathname,
  );
  if (path.endsWith("/")) {
    path += "index.html";
  }
  const localPath = resolveLocalPath(path);
  if (localPath === null) {
    response.writeHead(403).end();
    return;
  }
  let stat;
  try {
    stat = statSync(localPath);
  } catch {
    response.writeHead(404).end(`not found: ${path}`);
    return;
  }
  response.writeHead(200, {
    "Content-Type":
      contentTypes[extname(localPath)] ?? "application/octet-stream",
    "Content-Length": stat.size,
  });
  createReadStream(localPath).pipe(response);
}).listen(port, () => {
  console.log(`http://localhost:${port}/`);
});
