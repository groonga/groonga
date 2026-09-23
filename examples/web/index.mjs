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

import { HTTPFileSystem, open } from "../../lib/libgroonga-web.mjs";

const status = document.getElementById("status");
const output = document.getElementById("output");

// The files of the database are downloaded when they are used at
// first. We need only the URL of the directory that has them.
const fileSystem = new HTTPFileSystem(new URL("./database/", location));
const groonga = await open({ fileSystem });
const context = groonga.createContext();
context.openDatabase("/db/db");

status.textContent = "Ready";
// Tests wait for this.
document.body.dataset.groongaReady = "true";

function send(command) {
  const result = context.sendText(command);
  output.textContent = result;
  return result;
}

document.getElementById("form").addEventListener("submit", (event) => {
  event.preventDefault();
  send(document.getElementById("command").value);
});

// Tests use this.
globalThis.groongaSend = send;
send("status");
