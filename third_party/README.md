# Third-party code

| Library | Version | Licence | Used by | Source |
|---|---|---|---|---|
| [miniaudio](https://miniaud.io) | 0.11.22 | public domain or MIT No Attribution (see the end of `miniaudio.h`) | `connectors/desktop` (audio output on Windows, Linux, macOS) | <https://github.com/mackron/miniaudio/tree/0.11.22> |

Kept here, unmodified, so the desktop app builds offline with nothing to
install. To update it, replace the file with a newer release and run `make test`.
