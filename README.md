# unix-filesystem-simulator
Implementation of a Unix-like filesystem (ext2-based) with inode management, concurrency control and virtual disk simulation.

## Description

This project implements a simplified Unix-like filesystem inspired by **ext2**, developed in C.  
The filesystem is stored inside a virtual disk (a regular file) and supports file and directory operations, inode-based storage, and concurrent access simulation.

Key features:
- Indexed allocation using inodes (direct and indirect pointers)
- Hierarchical directory structure
- Support for hard links
- Permission management
- Concurrent access control using POSIX semaphores
- Simulation of multiple processes writing concurrently

## Project Structure

- `bloques.*` → Block-level access to the virtual disk  
- `ficheros_basico.*` → Low-level inode and block management  
- `ficheros.*` → File operations  
- `directorios.*` → Directory and path management  
- `semaforo_mutex_posix.*` → Concurrency control  
- `mi_*` → User commands (similar to UNIX tools):
  - `mi_ls`, `mi_cat`, `mi_mkdir`, `mi_rm`, `mi_stat`, etc.
- `simulacion.c` → Concurrent write simulation  
- `verificacion.c` → Verification of simulation results  
- `script*.sh` → Testing scripts  
- `docs/enunciado.pdf` → Original assignment statement

## Authors

- [@luzsalva](https://github.com/luzsalva)
- [@laaauriiiis](https://github.com/laaauriiiis)
- [@Niett204](https://github.com/Niett204)

This project was developed as part of a university assignment for 21718. Sistemes Operatius II (2023-24)
