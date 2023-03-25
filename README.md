xlog
====

xlog is a library implementing a binary append-only log. It is designed to be used in a distributed system, where multiple processes can write to the same log. It is also designed to be used in a system where the log is read by multiple processes, and where the log is read while it is being written to.

See [xlog.h](xlog.h) for the API.
