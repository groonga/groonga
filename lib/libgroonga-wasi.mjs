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

// A minimal WASI preview1 implementation with an in-memory file
// system. A Web browser doesn't provide WASI. This implements only
// what libgroonga.wasm needs.

const ESUCCESS = 0;
const EBADF = 8;
const EEXIST = 20;
const EINVAL = 28;
const EISDIR = 31;
const ENOENT = 44;
const ENOTDIR = 54;
const ENOTSUP = 58;

const FILETYPE_DIRECTORY = 3;
const FILETYPE_REGULAR_FILE = 4;

const OFLAGS_CREAT = 1 << 0;
const OFLAGS_DIRECTORY = 1 << 1;
const OFLAGS_EXCL = 1 << 2;
const OFLAGS_TRUNC = 1 << 3;

const WHENCE_SET = 0;
const WHENCE_CUR = 1;
const WHENCE_END = 2;

const PREOPENTYPE_DIR = 0;

const CLOCKID_MONOTONIC = 1;
const NANOSECONDS_PER_MILLISECOND = 1e6;

// A file in memory. The content grows on demand like a sparse file
// because Groonga creates a large file and writes a part of it.
class MemoryFile {
  constructor(content = new Uint8Array(0)) {
    this.content = content;
    this.size = content.length;
  }

  resize(size) {
    if (size > this.content.length) {
      const content = new Uint8Array(Math.max(size, this.content.length * 2));
      content.set(this.content.subarray(0, this.size));
      this.content = content;
    } else if (size > this.size) {
      // ftruncate() fills the extended part with zero. The part may
      // have the content that is removed by a previous resize().
      this.content.fill(0, this.size, size);
    }
    this.size = size;
  }

  read(offset, size) {
    if (offset >= this.size) {
      return new Uint8Array(0);
    }
    return this.content.subarray(offset, Math.min(offset + size, this.size));
  }

  write(offset, bytes) {
    if (offset + bytes.length > this.size) {
      this.resize(offset + bytes.length);
    }
    this.content.set(bytes, offset);
    return bytes.length;
  }
}

class MemoryDirectory {
  constructor() {
    this.entries = new Map();
  }
}

class OpenFile {
  constructor(path, node, isPreopen = false) {
    this.path = path;
    this.node = node;
    this.offset = 0;
    this.isPreopen = isPreopen;
  }
}

export class MemoryFileSystem {
  constructor() {
    this.root = new MemoryDirectory();
  }

  // Split "/a/b/c" into ["a", "b", "c"]. "." and "" are ignored.
  static splitPath(path) {
    return path.split("/").filter((name) => name !== "" && name !== ".");
  }

  resolve(path, createDirectories = false) {
    const names = MemoryFileSystem.splitPath(path);
    let directory = this.root;
    for (let i = 0; i < names.length - 1; i++) {
      let child = directory.entries.get(names[i]);
      if (child === undefined) {
        if (!createDirectories) {
          return null;
        }
        child = new MemoryDirectory();
        directory.entries.set(names[i], child);
      }
      if (!(child instanceof MemoryDirectory)) {
        return null;
      }
      directory = child;
    }
    return { directory, name: names[names.length - 1] ?? null };
  }

  lookup(path) {
    const resolved = this.resolve(path);
    if (resolved === null) {
      return null;
    }
    if (resolved.name === null) {
      return this.root;
    }
    return resolved.directory.entries.get(resolved.name) ?? null;
  }

  // The intermediate directories are created.
  addFile(path, content) {
    const resolved = this.resolve(path, true);
    const file = new MemoryFile(content);
    resolved.directory.entries.set(resolved.name, file);
    return file;
  }
}

export class WASI {
  constructor({
    args = [],
    env = {},
    fileSystem = new MemoryFileSystem(),
  } = {}) {
    this.args = args;
    this.env = env;
    this.fileSystem = fileSystem;
    this.instance = null;
    this.encoder = new TextEncoder();
    this.decoder = new TextDecoder();
    this.exitCode = null;
    this.openFiles = new Map();
    // 0, 1 and 2 are stdin, stdout and stderr. 3 is the preopened
    // root directory.
    this.openFiles.set(0, new OpenFile("/dev/stdin", null));
    this.openFiles.set(1, new OpenFile("/dev/stdout", null));
    this.openFiles.set(2, new OpenFile("/dev/stderr", null));
    this.openFiles.set(3, new OpenFile("/", fileSystem.root, true));
    this.nextFD = 4;
    this.onStandardOutput = null;
    this.onStandardError = null;
  }

  initialize(instance) {
    this.instance = instance;
    if (instance.exports._initialize) {
      instance.exports._initialize();
    }
  }

  get view() {
    return new DataView(this.instance.exports.memory.buffer);
  }

  get bytes() {
    return new Uint8Array(this.instance.exports.memory.buffer);
  }

  addFD(openFile) {
    const fd = this.nextFD++;
    this.openFiles.set(fd, openFile);
    return fd;
  }

  readString(pointer, size) {
    return this.decoder.decode(this.bytes.subarray(pointer, pointer + size));
  }

  readIOVectors(pointer, count) {
    const view = this.view;
    const vectors = [];
    for (let i = 0; i < count; i++) {
      const base = view.getUint32(pointer + i * 8, true);
      const size = view.getUint32(pointer + i * 8 + 4, true);
      vectors.push(this.bytes.subarray(base, base + size));
    }
    return vectors;
  }

  writeStandard(fd, vectors) {
    let size = 0;
    for (const vector of vectors) {
      size += vector.length;
      const callback = fd === 1 ? this.onStandardOutput : this.onStandardError;
      if (callback) {
        callback(this.decoder.decode(vector));
      }
    }
    return size;
  }

  getImportObject() {
    const wasi = this;
    return {
      wasi_snapshot_preview1: {
        args_get: (argvPointer, argvBufferPointer) => {
          const view = wasi.view;
          let offset = argvBufferPointer;
          for (const [i, argument] of wasi.args.entries()) {
            view.setUint32(argvPointer + i * 4, offset, true);
            const bytes = wasi.encoder.encode(argument + "\0");
            wasi.bytes.set(bytes, offset);
            offset += bytes.length;
          }
          return ESUCCESS;
        },
        args_sizes_get: (countPointer, sizePointer) => {
          const view = wasi.view;
          view.setUint32(countPointer, wasi.args.length, true);
          let size = 0;
          for (const argument of wasi.args) {
            size += wasi.encoder.encode(argument).length + 1;
          }
          view.setUint32(sizePointer, size, true);
          return ESUCCESS;
        },
        environ_get: (environPointer, environBufferPointer) => {
          const view = wasi.view;
          let offset = environBufferPointer;
          let i = 0;
          for (const [name, value] of Object.entries(wasi.env)) {
            view.setUint32(environPointer + i * 4, offset, true);
            const bytes = wasi.encoder.encode(`${name}=${value}\0`);
            wasi.bytes.set(bytes, offset);
            offset += bytes.length;
            i++;
          }
          return ESUCCESS;
        },
        environ_sizes_get: (countPointer, sizePointer) => {
          const view = wasi.view;
          const entries = Object.entries(wasi.env);
          view.setUint32(countPointer, entries.length, true);
          let size = 0;
          for (const [name, value] of entries) {
            size += wasi.encoder.encode(`${name}=${value}`).length + 1;
          }
          view.setUint32(sizePointer, size, true);
          return ESUCCESS;
        },
        clock_time_get: (id, precision, timePointer) => {
          // performance.now() may have a better resolution than
          // Date.now(). It's also monotonic.
          const milliseconds =
            id === CLOCKID_MONOTONIC ? performance.now() : Date.now();
          wasi.view.setBigUint64(
            timePointer,
            BigInt(Math.round(milliseconds * NANOSECONDS_PER_MILLISECOND)),
            true,
          );
          return ESUCCESS;
        },
        random_get: (pointer, size) => {
          crypto.getRandomValues(wasi.bytes.subarray(pointer, pointer + size));
          return ESUCCESS;
        },
        proc_exit: (code) => {
          wasi.exitCode = code;
          throw new Error(`proc_exit: <${code}>`);
        },
        fd_prestat_get: (fd, prestatPointer) => {
          const openFile = wasi.openFiles.get(fd);
          if (!openFile || !openFile.isPreopen) {
            return EBADF;
          }
          const view = wasi.view;
          view.setUint8(prestatPointer, PREOPENTYPE_DIR);
          const size = wasi.encoder.encode(openFile.path).length;
          view.setUint32(prestatPointer + 4, size, true);
          return ESUCCESS;
        },
        fd_prestat_dir_name: (fd, pathPointer, size) => {
          const openFile = wasi.openFiles.get(fd);
          if (!openFile || !openFile.isPreopen) {
            return EBADF;
          }
          const bytes = wasi.encoder.encode(openFile.path);
          wasi.bytes.set(bytes.subarray(0, size), pathPointer);
          return ESUCCESS;
        },
        fd_fdstat_get: (fd, fdstatPointer) => {
          const openFile = wasi.openFiles.get(fd);
          if (!openFile) {
            return EBADF;
          }
          const view = wasi.view;
          const isDirectory = openFile.node instanceof MemoryDirectory;
          view.setUint8(
            fdstatPointer,
            isDirectory ? FILETYPE_DIRECTORY : FILETYPE_REGULAR_FILE,
          );
          view.setUint16(fdstatPointer + 2, 0, true);
          // Allow all rights.
          view.setBigUint64(fdstatPointer + 8, ~0n, true);
          view.setBigUint64(fdstatPointer + 16, ~0n, true);
          return ESUCCESS;
        },
        fd_fdstat_set_flags: () => ESUCCESS,
        fd_filestat_get: (fd, filestatPointer) => {
          const openFile = wasi.openFiles.get(fd);
          if (!openFile) {
            return EBADF;
          }
          wasi.writeFilestat(filestatPointer, openFile.node);
          return ESUCCESS;
        },
        fd_filestat_set_size: (fd, size) => {
          const openFile = wasi.openFiles.get(fd);
          if (!openFile || !(openFile.node instanceof MemoryFile)) {
            return EBADF;
          }
          openFile.node.resize(Number(size));
          return ESUCCESS;
        },
        fd_close: (fd) => {
          if (!wasi.openFiles.has(fd)) {
            return EBADF;
          }
          wasi.openFiles.delete(fd);
          return ESUCCESS;
        },
        fd_seek: (fd, offset, whence, newOffsetPointer) => {
          const openFile = wasi.openFiles.get(fd);
          if (!openFile || !(openFile.node instanceof MemoryFile)) {
            return EBADF;
          }
          const value = Number(offset);
          switch (whence) {
            case WHENCE_SET:
              openFile.offset = value;
              break;
            case WHENCE_CUR:
              openFile.offset += value;
              break;
            case WHENCE_END:
              openFile.offset = openFile.node.size + value;
              break;
            default:
              return EINVAL;
          }
          wasi.view.setBigUint64(
            newOffsetPointer,
            BigInt(openFile.offset),
            true,
          );
          return ESUCCESS;
        },
        fd_read: (fd, iovectorsPointer, count, sizePointer) => {
          return wasi.readInternal(fd, iovectorsPointer, count, sizePointer);
        },
        fd_pread: (fd, iovectorsPointer, count, offset, sizePointer) => {
          return wasi.readInternal(
            fd,
            iovectorsPointer,
            count,
            sizePointer,
            Number(offset),
          );
        },
        fd_write: (fd, iovectorsPointer, count, sizePointer) => {
          return wasi.writeInternal(fd, iovectorsPointer, count, sizePointer);
        },
        fd_pwrite: (fd, iovectorsPointer, count, offset, sizePointer) => {
          return wasi.writeInternal(
            fd,
            iovectorsPointer,
            count,
            sizePointer,
            Number(offset),
          );
        },
        path_open: (
          dirFD,
          dirFlags,
          pathPointer,
          pathSize,
          oflags,
          rightsBase,
          rightsInheriting,
          fdFlags,
          fdPointer,
        ) => {
          const directory = wasi.openFiles.get(dirFD);
          if (!directory) {
            return EBADF;
          }
          const path = wasi.resolvePath(directory, pathPointer, pathSize);
          let node = wasi.fileSystem.lookup(path);
          if (oflags & OFLAGS_DIRECTORY) {
            if (node === null) {
              return ENOENT;
            }
            if (!(node instanceof MemoryDirectory)) {
              return ENOTDIR;
            }
          } else if (node === null) {
            if (!(oflags & OFLAGS_CREAT)) {
              return ENOENT;
            }
            node = wasi.fileSystem.addFile(path, new Uint8Array(0));
          } else if (oflags & OFLAGS_EXCL) {
            return EEXIST;
          } else if (node instanceof MemoryFile && oflags & OFLAGS_TRUNC) {
            node.resize(0);
          }
          const fd = wasi.addFD(new OpenFile(path, node));
          wasi.view.setUint32(fdPointer, fd, true);
          return ESUCCESS;
        },
        path_filestat_get: (
          dirFD,
          flags,
          pathPointer,
          pathSize,
          filestatPointer,
        ) => {
          const directory = wasi.openFiles.get(dirFD);
          if (!directory) {
            return EBADF;
          }
          const path = wasi.resolvePath(directory, pathPointer, pathSize);
          const node = wasi.fileSystem.lookup(path);
          if (node === null) {
            return ENOENT;
          }
          wasi.writeFilestat(filestatPointer, node);
          return ESUCCESS;
        },
        path_unlink_file: (dirFD, pathPointer, pathSize) => {
          const directory = wasi.openFiles.get(dirFD);
          if (!directory) {
            return EBADF;
          }
          const path = wasi.resolvePath(directory, pathPointer, pathSize);
          const resolved = wasi.fileSystem.resolve(path);
          if (resolved === null || resolved.name === null) {
            return ENOENT;
          }
          const node = resolved.directory.entries.get(resolved.name);
          if (node === undefined) {
            return ENOENT;
          }
          if (node instanceof MemoryDirectory) {
            return EISDIR;
          }
          resolved.directory.entries.delete(resolved.name);
          return ESUCCESS;
        },
        path_rename: (
          oldDirFD,
          oldPathPointer,
          oldPathSize,
          newDirFD,
          newPathPointer,
          newPathSize,
        ) => {
          const oldDirectory = wasi.openFiles.get(oldDirFD);
          const newDirectory = wasi.openFiles.get(newDirFD);
          if (!oldDirectory || !newDirectory) {
            return EBADF;
          }
          const oldPath = wasi.resolvePath(
            oldDirectory,
            oldPathPointer,
            oldPathSize,
          );
          const newPath = wasi.resolvePath(
            newDirectory,
            newPathPointer,
            newPathSize,
          );
          const oldResolved = wasi.fileSystem.resolve(oldPath);
          if (oldResolved === null || oldResolved.name === null) {
            return ENOENT;
          }
          const node = oldResolved.directory.entries.get(oldResolved.name);
          if (node === undefined) {
            return ENOENT;
          }
          const newResolved = wasi.fileSystem.resolve(newPath, true);
          oldResolved.directory.entries.delete(oldResolved.name);
          newResolved.directory.entries.set(newResolved.name, node);
          return ESUCCESS;
        },
        // Groonga doesn't use them on WASI. See GRN_HAVE_SOCKET in
        // lib/grn.h.
        poll_oneoff: () => ENOTSUP,
        sock_accept: () => ENOTSUP,
        sock_recv: () => ENOTSUP,
        sock_send: () => ENOTSUP,
        sock_shutdown: () => ENOTSUP,
      },
    };
  }

  resolvePath(directory, pathPointer, pathSize) {
    const path = this.readString(pathPointer, pathSize);
    if (path.startsWith("/")) {
      return path;
    }
    return `/${MemoryFileSystem.splitPath(`${directory.path}/${path}`).join(
      "/",
    )}`;
  }

  writeFilestat(pointer, node) {
    const view = this.view;
    const isDirectory = node instanceof MemoryDirectory;
    view.setBigUint64(pointer, 0n, true); // device
    view.setBigUint64(pointer + 8, 0n, true); // inode
    view.setUint8(
      pointer + 16,
      isDirectory ? FILETYPE_DIRECTORY : FILETYPE_REGULAR_FILE,
    );
    view.setBigUint64(pointer + 24, 1n, true); // number of links
    view.setBigUint64(pointer + 32, BigInt(isDirectory ? 0 : node.size), true);
    const nanoseconds = BigInt(Math.round(Date.now() * 1e6));
    view.setBigUint64(pointer + 40, nanoseconds, true); // access time
    view.setBigUint64(pointer + 48, nanoseconds, true); // modification time
    view.setBigUint64(pointer + 56, nanoseconds, true); // change time
  }

  readInternal(fd, iovectorsPointer, count, sizePointer, offset = null) {
    const openFile = this.openFiles.get(fd);
    if (!openFile) {
      return EBADF;
    }
    if (openFile.node instanceof MemoryDirectory) {
      return EISDIR;
    }
    if (!(openFile.node instanceof MemoryFile)) {
      // Nothing to read from stdin.
      this.view.setUint32(sizePointer, 0, true);
      return ESUCCESS;
    }
    let position = offset === null ? openFile.offset : offset;
    let size = 0;
    for (const vector of this.readIOVectors(iovectorsPointer, count)) {
      const content = openFile.node.read(position, vector.length);
      vector.set(content);
      position += content.length;
      size += content.length;
      if (content.length < vector.length) {
        break;
      }
    }
    if (offset === null) {
      openFile.offset = position;
    }
    this.view.setUint32(sizePointer, size, true);
    return ESUCCESS;
  }

  writeInternal(fd, iovectorsPointer, count, sizePointer, offset = null) {
    const openFile = this.openFiles.get(fd);
    if (!openFile) {
      return EBADF;
    }
    const vectors = this.readIOVectors(iovectorsPointer, count);
    if (!(openFile.node instanceof MemoryFile)) {
      const size = this.writeStandard(fd, vectors);
      this.view.setUint32(sizePointer, size, true);
      return ESUCCESS;
    }
    let position = offset === null ? openFile.offset : offset;
    let size = 0;
    for (const vector of vectors) {
      // The vector refers the WebAssembly memory. write() may grow
      // the memory of the file system. So we must copy it.
      openFile.node.write(position, vector.slice());
      position += vector.length;
      size += vector.length;
    }
    if (offset === null) {
      openFile.offset = position;
    }
    this.view.setUint32(sizePointer, size, true);
    return ESUCCESS;
  }
}
