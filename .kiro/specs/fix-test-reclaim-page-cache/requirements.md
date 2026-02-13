# Requirements Document

## Introduction

This document specifies the requirements for fixing three correctness issues in the `TestReclaimFilePageCache` test located in `src/unit/test_util.cpp`. The test validates the `reclaimFilePageCache()` function which uses `fadvise` to evict pages from the Linux page cache. The current implementation contains bugs that could cause false test passes, incorrect test behavior, and resource leaks.

## Glossary

- **Test_Util**: The test file `src/unit/test_util.cpp` containing utility function tests
- **cache_exist**: A helper function that uses `mmap()` and `mincore()` to check if a file's pages are resident in memory
- **MAP_FAILED**: The error return value from `mmap()`, defined as `(void*)-1`, distinct from `nullptr`
- **tmpfs**: A temporary filesystem that stores files in volatile memory rather than on disk
- **TMPFS_MAGIC**: A constant (`0x01021994`) identifying tmpfs filesystem type
- **Page_Cache**: The kernel's cache of file data in memory, which `reclaimFilePageCache()` evicts

## Requirements

### Requirement 1: Correct mmap() Error Checking

**User Story:** As a developer, I want the `cache_exist()` function to correctly detect `mmap()` failures, so that the test does not operate on invalid memory mappings.

#### Acceptance Criteria

1. WHEN `mmap()` fails, THE cache_exist function SHALL detect the failure by comparing the return value against `MAP_FAILED` (not `nullptr`)
2. WHEN `mmap()` returns `MAP_FAILED`, THE cache_exist function SHALL fail the test assertion before calling `mincore()` or `munmap()`
3. WHEN `mmap()` succeeds, THE cache_exist function SHALL proceed to call `mincore()` and `munmap()` with the valid mapping

### Requirement 2: Correct tmpfs Detection Logic

**User Story:** As a developer, I want the test to skip execution on tmpfs filesystems, so that the test only runs on filesystems where page-cache semantics are meaningful.

#### Acceptance Criteria

1. WHEN `/tmp` is backed by tmpfs, THE TestReclaimFilePageCache test SHALL skip execution (page-cache eviction semantics differ on tmpfs)
2. WHEN `/tmp` is backed by a regular disk-based filesystem, THE TestReclaimFilePageCache test SHALL proceed with execution
3. WHEN `statfs()` fails to determine the filesystem type, THE TestReclaimFilePageCache test SHALL proceed with execution (assume regular filesystem)

### Requirement 3: Proper File Descriptor Management

**User Story:** As a developer, I want all file descriptors to be properly closed, so that the test does not leak system resources.

#### Acceptance Criteria

1. WHEN the test completes successfully, THE TestReclaimFilePageCache test SHALL close the file descriptor before returning
2. WHEN the test skips due to tmpfs detection, THE TestReclaimFilePageCache test SHALL not leak any file descriptors (fd is opened after the tmpfs check)
3. WHEN the test encounters an assertion failure after opening the file, THE TestReclaimFilePageCache test SHALL ensure the file descriptor is closed (via test cleanup or explicit close)
