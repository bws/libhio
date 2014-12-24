/* -*- Mode: C; c-basic-offset:2 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2014      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

/**
 * @file hio_api.h
 * @brief API for libhio
 *
 * This file describes the design and API of libhio. This library is intended to
 * provide an extensible API for writing to hierarchal data storage systems.
 */

/**
 * @mainpage Introduction
 *
 * @section about About libhio
 *
 * libhio is an open-source library intended for writing data to hierarchal data
 * store systems. These systems may be comprised of one or more logical layers
 * including parallel file systems (PFS), burst buffers (BB), and local memory.
 * libhio provides support for automated fall-back on alternate destinations if
 * part of the storage hierarchy becomes unavailable.
 *
 * @section goals Goals
 *
 * High performance systems are rapidly increasing in size and complexity. To
 * keep up with the IO demands of applications IO subsystems are also
 * increasing in complexity. libhio is designed to insulate applications from the
 * rapid evolution of IO systems by providing a simple, easy to integrate,
 * POSIX-like interface to provide applications with a smooth transition from parallel
 * file systems (Lustre, Panasas) to future IO architectures. libhio is
 * intended to improve IO of applications by implementing IO best practices once
 * instead of per-application.
 *
 * An additional design goal of libhio is to be easily extensible to new IO
 * architectures, programming models, and higher-level languages. Support for
 * new architectures is handled by the runtime selection of IO backends. This
 * includes support for proprietary (binary-only) vendor supplied backends to
 * support specific IO hardware.
 *
 * @section features Features
 *
 * libhio provides the following features:
 *
 * - An interface that is always thread-safe. No application locking will be
 *   necessary to use libhio.
 * - A simple interface that provides a minimal POSIX-like IO interface. This
 *   is intended to make libhio easier to integrate into existing applications.
 *   POSIX semantics will be weakened to allow for optimization within libhio.
 * - Allows for user specification of filesystem specific "hints" while
 *   providing good defaults.
 * - A configuration interface that allows applications to specify
 *   configuration options via the environment, API calls, and file parsing.
 *   The file parsing support includes support for reading options from
 *   a user-specified file. More on configuration can be found in @ref config.
 * - Supports transparent (when possible) fall back on other destinations if
 *   part of the IO hierarchy fails. For example, falling back on a parallel
 *   file system on failure of a burst buffer.
 * - Provide full support for existing IO usage models including n -\> 1, n -\> n,
 *   and n -\> m.
 * - An abstract namespace with support to export to a single or multiple
 *   POSIX files.
 * - An interface to provide applications with the optimal checkpointing interval.
 *   This will take into account the file size, current system stability, and
 *   the current destination.
 * - Support for managing available burst buffer space.
 * - Support for scheduling automatic drain from temporary data roots (BB, local
 *   memory) to more permanent data roots (PFS).
 * - Improves performance by following best practice for HPC IO. This includes
 *   support for IO refactoring, turnstiling, and spreading IO over multiple
 *   destination directories.
 * - Provide support for querying performance characteristics.
 *
 * @section timeline Release Timeline
 *
 * The initial beta release of libhio will be available on or around December 1, 2014.
 * This release will support the full API with PFS data roots. A version
 * supporting Trinity's burst buffer architecture will be available no later
 * than April 3, 2015.
 *
 * @section iointerception IO Interception
 *
 * At this time libhio will not provide support for the interception and redirection
 * of POSIX IO calls (open, close read, write). We are investigating providing this
 * capability in a future version.
 *
 * @section namespace Namespace
 *
 * libhio provides an abstract namespace to enable performance improvements and abstract
 * out destination implementation details. The libhio file namespace is broken down into
 * three components:
 *
 * -# Context: All data managed by an hio instance.
 *
 * -# Dataset: A complete collection of files associated with a particular type of data.
 *   For example, all files of an n -\> n restart file.
 *
 * -# ID: Particular instance of a dataset. This is expected to be an increasing sequence
 *   and will be used if the latest instance is requested.
 *
 * Interfaces are provided to move from the abstract namespace to a POSIX namespace.
 *
 * @subsection data_layout Data Layout
 *
 * In general libhio provides no guarantees of the physical structure of a file. In
 * the case of POSIX-like filesystems, however, the initial implementation of libhio
 * will store all data associated with a context, dataset, id triple with the path:
 *
 *    context_name.hio/dataset_name/id/
 *
 * These directories contain file data as well as a manifest describing the mapping
 * from the hio file to the application file(s). To read a particular libhio file this
 * directory structure is required. Future releases of libhio may re-factor this output.
 *
 * @section data_roots Data Roots
 *
 * libhio data destinations are knows as data roots. The data roots supported by libhio
 * are parallel file systems (Lustre, Panasas) and burst buffers. Support for additional
 * data roots will be added as needed. libhio supports automatic migration of any complete
 * dataset from a burst buffer data root to a parallel file system.
 *
 * The applications can specify multiple data roots for a context. In the event of a failure
 * libhio will notify the application and allow the operation to fall back to an alternate
 * data root. The objective is to allow the application to make progress when possible in
 * the face of filesystem failures with minimal logic embedded in the application itself. The
 * currently active data root will influence the checkpoint interval recommended by
 * hio_should_checkpoint().
 *
 * @section sec_configuration Configuration Interface
 *
 * libhio provides a flexible configuration interface. The basic units
 * of configuration used by libhio for configuration are control
 * variables. Control variables are simple "key = value" pairs that
 * allow applications to control libhio behavior and fine-tuning
 * performance. A control variable may apply globally or to specific HIO
 * objects such as contexts or datasets. Control variables can be set via environment
 * variables, API calls (see hio_config_set_value()), or configuration
 * files. For scalability, all configuration is applied at rank 0 of
 * the context communicator and is then propagated to all other ranks.
 * If any variable is set via multiple mechanisms the final value will
 * be set according to the following precedence:
 *
 * - System administrator overrides (highest)
 * - libhio API calls
 * - Environment variables of the form HIO_variable_name
 * - Configuration files (lowest)
 *
 * Per context or dataset specific values for variables take precedence over globally
 * set values.
 *
 * @subsection subsec_configuration_file File Configuration
 *
 * Configuration files can be used to set variable values globally or within a
 * specific context or dataset. Configuration files are specified at context creation
 * (see hio_init_single() and hio_init_mpi(). They can be divided into sections using keywords
 * specified within []'s. The keywords recognized by libhio are: global, context:context_name, or
 * dataset:data_set_name. Variable values not within a section, by
 * default, apply globally.
 *
 * Example: @code
   # these are global
       context_base_verbose = 10
   [global]
   # these are also global
       context_data_roots = BB,PFS:/lscatch1,/lscratch2
   [context:foo]
   # apply only to context "foo"
       context_data_roots = PFS:/lscratch1
   [dataset:restart]
   # apply to dataset "restart" in any context
       dataset_stripe_width = 4M
@endcode
 *
 * If desired an application can set a prefix for any line parsed by libhio. This allows the
 * application to add hio specific configuration to an existing file.
 *
 * Example (Prefix HIO): @code
   HIO [global]
   HIO     context_data_roots = BB,PFS:/lscatch1,/lscratch2
   HIO [context:foo]
   HIO     context_data_roots = PFS:/lscratch1
   HIO [dataset:restart]
   HIO     dataset_stripe_width = 4M
@endcode
 *
 * Any characters appearing after a # character are treated as comments and
 * ignored by libhio unless the # character appears at the beginning of the line
 * and is part of the application specified line prefix (eg #HIO).
 *
 * @subsection subsec_configuration_env Environment Configuration
 *
 * Environment variables can be used to set variable values. libhio environment variables
 * are of the form:
 *
 * - HIO_\<variable_name\> - For global values.
 * - HIO_context_\<context_name\>_\<variable_name\> - For context specific variables.
 * - HIO_dataset_\<dataset_name\>_\<variable_name\> - For dataset specific variables on all contexts.
 * - HIO_dataset_\<context_name\>_\<dataset_name\>_\<variable_name\> - For dataset specific variables on a
 *   specific context.
 *
 * @subsection subsec_configuration_vars Configuration Variables
 *
 * This section lists the configuration variables supported by libhio. Variable names starting with
 * context_ apply to contexts and dataset_ apply to datasets. Other variables may apply to one or
 * both. The following subsections list the available configuration options. This list may be
 * incomplete and additional variables will be added in future releases of this document.
 *
 * @subsubsection subsubsec_configuration_context Context Specific Variables
 *
 * - @b context_data_roots - List of data roots to use for this context. This is a comma-delimited
 *   list including any of the following: BB (burst buffer), or PFS:path (parallel file
 *   system).
 *
 * - @b context_checkpoint_size - Expected size of checkpoint data. This is used to help libhio determine the
 *   appropriate checkpoint interval for the job.
 *
 * - @b context_print_statistics - Print IO statistics when hio_fini() is called.
 *
 * - @b context_base_verbose - Verbosity level of libhio (0-100). The default is a verbosity level of 0
 *   which outputs errors only from libhio. Higher levels will output more and more detailed
 *   debugging information.
 *
 * @subsubsection subsubsec_configuration_dataset Dataset Specific Variables
 *
 * - @b dataset_stage_mode - Mode to use for staging files to more permanent data stores (ex: BB \-\> PFS).
 *   Available modes: lazy (stage periodically or at end of job), immediate (stage when the dataset
 *   is closed).
 *
 * - @b dataset_stripe_width - Filesystem striping width. Passed to the underlying filesystem if supported.
 *
 * @page page_example Examples
 * @section sec_example_c C Example
 * @include example.c
 */

#if !defined(HIO_H)
#define HIO_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

/**
 * @ingroup API
 * @brief HIO library context
 *
 * HIO contexts are used to identify specific instances of the hio library.
 * Instances are created with hio_init_single() or hio_init_mpi() and destroyed with
 * hio_finalize(). It is possible to have multiple active contexts at any time
 * during execution. This type is opaque.
 */
typedef struct hio_context_t *hio_context_t;

/**
 * @ingroup API
 * @brief HIO dataset handle
 *
 * HIO datasets represent one or more HIO "files" on the data store. This type
 * is opaque.
 */
typedef struct hio_dataset_t *hio_dataset_t;

/**
 * @ingroup API
 * @brief HIO element handle
 *
 * HIO elements are the primary mechanism for performing I/O using libhio. A
 * libhio element may or may not correspond to a specific file on a traditional
 * POSIX filesystem. This type is opaque.
 *
 * The special value HIO_ELEMENT_NULL is a sentinel value used for
 * non-existent elements.
 */
typedef struct hio_element_t *hio_element_t;

/**
 * @ingroup API
 * @brief hio I/O request
 *
 * hio requests are the primary mechanism for checking for completion of
 * non-blocking I/O requests. This type is opaque.
 *
 * The special value HIO_REQUEST_NULL is a sentinel value used for
 * non-existent requests.
 */
typedef struct hio_request_t *hio_request_t;

/**
 * @ingroup API
 * @brief hio object handle
 *
 * This type is used as a placeholder for any hio object including
 * hio_context_t, hio_dataset_t, and hio_element_t.
 */
typedef struct hio_object_t *hio_object_t;

/**
 * @ingroup API
 * @brief NULL element
 */
#define HIO_ELEMENT_NULL NULL

/**
 * @ingroup API
 * @brief NULL request
 */
#define HIO_REQUEST_NULL NULL

/**
 * @ingroup API
 * @brief Default configuration file
 */
#define HIO_CONFIG_FILE_DEFAULT (char *) -2

/**
 * @ingroup API
 * @brief Highest available dataset id
 */
#define HIO_DATASET_ID_LAST (int64_t) -1

/**
 * @ingroup errorhandling
 * @brief Error codes that may be returned by libhio routines
 *
 * libhio functions generally return 0 on success and a negative number on
 * failure.
 */
typedef enum hio_return_t {
  /** The hio operation completed successfully */
  HIO_SUCCESS             = 0,
  /** Generic hio error */
  HIO_ERROR               = -1,
  /** Permissions error or operation not permitted */
  HIO_ERR_PERM            = -2,
  /** Short read or write */
  HIO_ERR_TRUNCATE        = -3,
  /** Out of resources */
  HIO_ERR_OUT_OF_RESOURCE = -4,
  /** Not found */
  HIO_ERR_NOT_FOUND       = -5,
  /** Not available */
  HIO_ERR_NOT_AVAILABLE   = -6,
  /** Bad parameter */
  HIO_ERR_BAD_PARAM       = -7,
  /** Temporary IO error. Try the IO again later. */
  HIO_ERR_IO_TEMPORARY   = -0x00010001,
  /** Permanent IO error. IO to the current data root is no longer available. */
  HIO_ERR_IO_PERMANENT   = -0x00010002,
} hio_return_t;

/**
 * @ingroup API
 * @brief Dataset open flags
 */
typedef enum hio_flags_t {
  /** Open the dataset read-only Can not be combined with
   * HIO_FLAG_WRONLY. */
  HIO_FLAG_RDONLY   = 0,
  /** Open the dataset write-only. Can not be combined with
   * HIO_FLAG_RDONLY. */
  HIO_FLAG_WRONLY   = 1,
  /** Create a new element */
  HIO_FLAG_CREAT    = 64,
  /** Remove all existing data associated with the dataset */
  HIO_FLAG_TRUNC    = 512,
  /** Append to an existing dataset */
  HIO_FLAG_APPEND   = 1024,
} hio_flags_t;

/**
 * @ingroup API
 * @brief Flush modes
 */
typedef enum hio_flush_mode_t {
  /** Locally flush data. This mode ensures that the user buffers can
   * be reused by the application. It does not ensure the data has
   * been written out to the backing store. */
  HIO_FLUSH_MODE_LOCAL    = 0,
  /** Ensure all data has been written out to the backing store. */
  HIO_FLUSH_MODE_COMPLETE = 1,
} hio_flush_mode_t;

/**
 * @ingroup configuration
 * @brief Configuration variable types
 */
typedef enum hio_config_type_t {
  /** C99/C++ boolean. Valid values: true, false, 0, 1 */
  HIO_CONFIG_TYPE_BOOL,
  /** NULL-terminated string */
  HIO_CONFIG_TYPE_STRING,
  /** 32-bit signed integer */
  HIO_CONFIG_TYPE_INT32,
  /** 32-bit unsigned integer */
  HIO_CONFIG_TYPE_UINT32,
  /** 64-bit signed integer */
  HIO_CONFIG_TYPE_INT64,
  /** 64-bit unsigned integer */
  HIO_CONFIG_TYPE_UINT64,
  /** IEEE-754 floating point number */
  HIO_CONFIG_TYPE_FLOAT,
  /** IEEE-754 double-precision floating point number */
  HIO_CONFIG_TYPE_DOUBLE,
} hio_config_type_t;

/**
 * @ingroup API
 * @brief Dataset element modes
 */
typedef enum hio_dataset_mode_t {
  /** Element(s) in the dataset have unique offset spaces. This mode
   * is equivalent to an N-N IO pattern where N is the number of
   * ranks in the communicator used to create the hio context. */
  HIO_SET_ELEMENT_UNIQUE,
  /** Element(s) in the dataset have shared offset spaces. This mode
   * is equivalent to an N-1 IO pattern where N is the number of
   * ranks in the communicator used to create the hio context. */
  HIO_SET_ELEMENT_SHARED,
} hio_dataset_mode_t;

/**
 * @ingroup API
 * @brief Checkpoint recommendations
 */
typedef enum hio_recommendation_t {
  /** Do not attempt to checkpoint at this time */
  HIO_SCP_NOT_NOW,
  /** Checkpoint strongly recommended */
  HIO_SCP_MUST_CHECKPOINT,
} hio_recommendation_t;


/**
 * @ingroup API
 * @brief Create a new single-process hio context
 *
 * @param[out] new_context         newly created hio context
 * @param[in]  config_file         config file for this context
 * @param[in]  config_file_prefix  prefix proceeding all config file lines
 * @param[in]  context_name        context name
 *
 * @returns HIO_SUCCESS if the context has been opened successfully
 * @returns HIO_ERROR if any error occurs
 *
 * This function creates a new single-process hio context and returns it to the caller.
 * No other processes should attempt to modify any datasets open within the context.
 * Depending on the semantics of the underlying filesystems other processes may not get
 * an error if they attempt to open the same context.
 *
 * When initializing a context a configuration file can be specified. This file should
 * contain the configuration for the context and all datasets in the context. If NULL
 * is specified in {config_file} then no configuration file will be used. The special
 * value HIO_CONFIG_FILE_DEFAULT will make libhio look for a file named context_name.cfg.
 * For more on configuration see @ref sec_configuration.
 */
int hio_init_single (hio_context_t *new_context, const char *config_file, const char *config_file_prefix,
                     const char *context_name);

/**
 * @ingroup API
 * @brief Create a new parallel hio context
 *
 * @param[out] new_context         newly created hio context
 * @param[in]  comm                pointer to an MPI communicator (may be NULL)
 * @param[in]  config_file         config file for this context
 * @param[in]  config_file_prefix  prefix proceeding all config file lines
 * @param[in]  name                context name
 *
 * @returns HIO_SUCCESS if the context has been opened successfully
 * @returns HIO_ERROR if any error occurs
 *
 * This function creates a new parallel hio context and returns it to the caller. This 
 * function is collective and must be called by all processes in the MPI communicator
 * pointed to by {comm}. The communicator pointed to by {comm} must contain all
 * processes that will perform IO in this context. The caller is free to use or free
 * {comm} after the call has returned. Processes outside {comm} should not attempt
 * to modify any datasets open within this context. If {comm} is NULL then hio will
 * attempt to use MPI_COMM_WORLD.
 *
 * When initializing a context a configuration file can be specified. This file should
 * contain the configuration for the context and all datasets in the context. If NULL
 * is specified in {config_file} then no configuration file will be used. The special
 * value HIO_CONFIG_FILE_DEFAULT will make libhio look for a file named context_name.cfg.
 * For more on configuration see @ref sec_configuration.
 */
/* Check for MPI */
#if !defined(MPI_VERSION)
int hio_init_mpi (hio_context_t *new_context, void *comm, const char *config_file,
                  const char *config_file_prefix, const char *name);
#else
int hio_init_mpi (hio_context_t *new_context, MPI_Comm *comm, const char *config_file,
                  const char *config_file_prefix, const char *name);
#endif

/**
 * @ingroup API
 * @brief Finalize an hio context
 *
 * @param[in,out] ctx  hio context to finalize
 *
 * @returns hio_return_t
 *
 * This function finalizes and frees all memory associated with an hio context.
 * It is erroneous to call this function while there are any outstanding open
 * datasets in the context.
 */
int hio_fini (hio_context_t *ctx);

/**
 * @ingroup error_handling
 * @brief Get a string representation of the last error
 *
 * @param[in]  ctx    hio context
 * @param[out] error  string representation of the last error
 *
 * This function stores a pointer to the string representation of the most recent
 * error that occurred on a given context in {error}. Multiple calls to this
 * function will return each a prior error until all errors have been reported. If no
 * error has occurred this function will store NULL in {error}. It is the
 * responsibility of the caller to free the pointer returned in {error}. The error
 * string will be prefixed with useful information such as the time, date, node,
 * and rank. Note that the absolute ordering of errors is not specified if the
 * application is using libhio from multiple threads.
 */
int hio_err_get_last (hio_context_t ctx, char **error);

/**
 * @ingroup error_handling
 * @brief Print 
 *
 * @param[in]  ctx     hio context
 * @param[in]  output  output file handle
 * @param[in]  format  print format (see printf)
 * @param[in]  ...     format values
 *
 * This function prints the application's error string followed by a
 * string representation of the hio error last seen on this thread.
 * Like hio_err_get_last() this function dequeues the last error.
 */
int hio_err_print_last (hio_context_t ctx, FILE *output, char *format, ...);

/**
 * @ingroup error_handling
 * @brief Print
 *
 * @param[in]  ctx     hio context
 * @param[in]  output  output file handle
 * @param[in]  format  print format (see printf)
 * @param[in]  ...     format values
 *
 * This function prints each hio error that has occurred on this thread preceded by
 * the string specified by {format} and {...}. This call dequeues all errors
 * that are pending being reported.
 */
int hio_err_print_all (hio_context_t ctx, FILE *output, char *format, ...);

/**
 * @ingroup API
 * @brief Open/create an hio dataset
 *
 * @param[in]  ctx      hio context
 * @param[out] set_out  hio dataset handle
 * @param[in]  name     name of hio dataset
 * @param[in]  set_id   identifier for this set. eg. step number
 * @param[in]  flags    open flags
 * @param[in]  mode     dataset mode (unique or shared)
 *
 * @returns HIO_SUCCESS on success
 * @returns HIO_ERROR_PARAM if a bad parameter is supplied. This can happen if
 * an existing dataset is opened with the incorrect mode.
 *
 * This function attempts to open/create an hio dataset. A dataset represents a
 * collection of one or more related elements. For example, a set of elements associated
 * with an n-n IO pattern. In the case of n-n the caller must specify different element
 * names on all ranks when calling to hio_open(). This call is collective and must
 * be made by all ranks in the context. There is no restriction on the number
 * of elements opened by any rank. This number is only limited by the resources
 * available. If the special value HIO_DATASET_ID_LAST is specified in {set_id} libhio
 * will attempt to open an existing dataset with the maximum id.
 *
 * libhio does not currently support opening an existing dataset, id pair with
 * a different mode than it was created with.
 */
int hio_dataset_open (hio_context_t ctx, hio_dataset_t *set_out, const char *name,
                      int64_t set_id, hio_flags_t flags, hio_dataset_mode_t mode);

/**
 * @ingroup API
 * @brief Close an hio dataset
 *
 * @param[in,out] set  hio dataset handle
 *
 * @returns hio_return_t
 *
 * This function closes and releases all resources associated with the dataset.
 * It is erroneous to call this function on a dataset while there are outstanding
 * elements open locally in the dataset.
 */
int hio_dataset_close (hio_dataset_t *set);

/**
 * @ingroup API
 * @brief Unlink an hio dataset
 *
 * @param[in] ctx     hio context
 * @param[in] name    name of hio dataset
 * @param[in] set_id  identified for the dataset
 *
 * @returns hio_return_t
 *
 * This function removes all data associated with an hio dataset on all data
 * roots.
 */
int hio_dataset_unlink (hio_context_t ctx, const char *name, int64_t set_id);

/**
 * @ingroup API
 * @brief Open an element
 *
 * @param[in]  dataset      hio dataset this element belongs to
 * @param[out] element_out  new hio element handle
 * @param[in]  element_name file to open
 * @param[in]  flags        open flags
 *
 * @returns hio_return_t
 *
 * This function attempts to open an hio element. An hio element may be represented
 * by a single or many files depending on the job size, number of writers, and
 * configuration. On return the element handle can be used even though the element
 * may not be open. If an error occurred during open it may be returned by another
 * call (hio_read, hio_write, etc). This call is not collective and can be called
 * from any rank. Calls to open the same element from multiple ranks is allowed unless
 * exclusive access (see @ref HIO_FLAG_EXCL) is requested.
 */
int hio_open (hio_dataset_t dataset, hio_element_t *element_out, const char *element_name,
              hio_flags_t flags);

/**
 * @ingroup API
 * @brief Construct a traditional file out of an hio dataset
 *
 * @param[in] dataset      hio dataset handle
 * @param[in] destination  destination file or directory name
 * @param[in] flags        construct flags
 *
 * This function takes an hio dataset and reconstructs the file(s) that
 * make up the hio element. This could be a single file or many depending
 * on how the hio element was constructed. If the dataset has unique offsets
 * for each rank the resulting files will have the form \<element_name\>.\<rank\>.
 * This function will fail if the output is a directory and a file already
 * exists at {destination}. This functionality is still in development and
 * will likely change before the final version of this API is released.
 *
 * The functionality of this API will also be supplied by the command-line
 * executable hio_construct.
 *
 * We also plan to add support for converting a POSIX file(s) into an hio
 * dataset. This functionality is also under development pending the
 * finalization of the rest of the libhio API.
 */
int hio_dataset_construct (hio_dataset_t dataset, const char *destination, int flags);


/**
 * @ingroup API
 * @brief Close an open element
 *
 * @param[in,out] element Open hio element handle
 *
 * @returns hio_return_t
 *
 * This function finalizes all outstanding I/O on an open element. On success
 * {hio_element} is set to HIO_ELEMENT_NULL.
 */
int hio_close (hio_element_t *element);

/**
 * @ingroup blocking
 * @brief Start a blocking contiguous write to an hio element
 *
 * @param[in]  element      hio element handle
 * @param[in]  offset       offset to write to
 * @param[in]  reserved0    reserved for future use (pass 0)
 * @param[in]  ptr          data to write
 * @param[in]  count        number of elements to write
 * @param[in]  size         size of each element
 *
 * @returns the number of bytes written if the write was successful
 *
 * This function writes contiguous data from ptr to the element specified in
 * {element}. The call returns when the buffer pointed to by {ptr} is no
 * longer needed by hio and is free to be modified. Completion of a write
 * does not guarantee the data has been written to the data store.
 */
ssize_t hio_write (hio_element_t element, off_t offset, unsigned long reserved0, void *ptr,
                   size_t count, size_t size);


/**
 * @ingroup nonblocking
 * @brief Start a non-blocking contiguous write to an hio element
 *
 * @param[in]  element      hio element handle
 * @param[out] request      new hio request (may be NULL)
 * @param[in]  offset       offset to write to
 * @param[in]  reserved0    reserved for future use (pass 0)
 * @param[in]  ptr          data to write
 * @param[in]  count        number of elements to write
 * @param[in]  size         size of each element
 *
 * @returns HIO success if the write has successfully been scheduled or completed
 *
 * This function schedules a non-blocking write of contiguous data from {ptr} to the
 * element specified in {element}. The call returns immediately even if the
 * write has not completed. If the request is not yet complete and a non-NULL
 * value is specified in {request} this function will return an hio request in
 * {request}. The application is requires to call one of hio_test(), hio_wait(),
 * or hio_join() on the returned request to ensure all resources are freed. If a
 * NULL value is specified in {request} any error occurring during the write
 * will be reported by either hio_flush() or hio_flush_all().
 * The hio implementation is free to return HIO_REQUEST_NULL if the write is complete.
 * In the context of writes a request is complete when the buffer specified by
 * {ptr} is free to be modified. Completion of a write request does not guarantee
 * the data has been written.
 */
int hio_write_nb (hio_element_t element, hio_request_t *request, off_t offset,
                  unsigned long reserved0, void *ptr, size_t count, size_t size);

/**
 * @ingroup blocking
 * @brief Start a blocking strided write to an hio element
 *
 * @param[in]  element      hio element handle
 * @param[in]  offset       offset to write to
 * @param[in]  reserved0    reserved for future use (pass 0)
 * @param[in]  ptr          data to write
 * @param[in]  count        number of elements to write
 * @param[in]  size         size of each element
 * @param[in]  stride       stride between each element
 *
 * @returns the number of bytes written if the write was successful
 *
 * This function writes strided data specified by {ptr}, {size}, and {stride}
 * to the element specified in {element}. The call returns when the
 * buffer pointed to by {ptr} is no longer needed by hio and is free to be
 * modified. Completion of a write does not guarantee the data has been
 * written to the data store.
 */
int hio_write_strided (hio_element_t element, off_t offset, unsigned long reserved0,
                       void *ptr, size_t count, size_t size, size_t stride);

/**
 * @ingroup nonblocking
 * @brief Start a non-blocking strided write to an hio element
 *
 * @param[in]  element      hio element handle
 * @param[out] request      new hio request (may be NULL)
 * @param[in]  offset       offset to write to
 * @param[in]  reserved0    reserved for future use (pass 0)
 * @param[in]  ptr          data to write
 * @param[in]  count        number of elements to write
 * @param[in]  size         size of each element
 * @param[in]  stride       stride between each element
 *
 * @returns HIO success if the write has successfully been scheduled or completed
 *
 * This function schedules a non-blocking write of strided data specified by {ptr},
 * {size}, and {stride} to the
 * element specified in {element}. The call returns immediately even if the
 * write has not completed. If the request is not yet complete and a non-NULL
 * value is specified in {request} this function will return an hio request in
 * {request}. The application is requires to call one of hio_test(), hio_wait(),
 * or hio_join() on the returned request to ensure all resources are freed. If a
 * NULL value is specified in {request} any error occurring during the write
 * will be reported by either hio_flush() or hio_flush_all().
 * The hio implementation is free to return HIO_REQUEST_NULL if the write is complete.
 * In the context of writes a request is complete when the buffer specified by
 * {ptr} is free to be modified. Completion of a write request does not guarantee
 * the data has been written.
 */
int hio_write_strided_nb (hio_element_t element, hio_request_t *request, off_t offset,
                          unsigned long reserved0, void *ptr, size_t count, size_t size, size_t stride);

/**
 * @ingroup nonblocking
 * @brief Complete all pending writes on all elements of a dataset
 *
 * @param[in] element      hio element handle
 * @param[in] mode         flush mode
 *
 * @returns HIO_SUCCESS if all pending writes completed successfully
 * @returns the error code of the first failed write
 *
 * This function completes all outstanding writes on the specified element. The value
 * specified in {mode} indicates the level of completion requested. A mode of
 * HIO_FLUSH_MODE_LOCAL ensures that all application buffers are free to be reused.
 * A mode of HIO_FLUSH_MODE_COMPLETE ensures that the data has been flushed to the
 * current active data root.
 *
 * This function will return an error code if any write on the specified hio element
 * can not complete.
 */
int hio_flush (hio_element_t element, hio_flush_mode_t mode);

/**
 * @ingroup nonblocking
 * @brief Complete all pending writes on all elements of a dataset
 *
 * @param[in] dataset  hio dataset handle
 * @param[in] mode     flush mode
 *
 * @returns HIO_SUCCESS if all pending writes completed successfully
 * @returns the error code of the first failed write
 *
 * This function completes all outstanding writes on all open elements in a dataset.
 * The value specified in {mode} indicates the level of completion requested. A mode
 * of HIO_FLUSH_MODE_LOCAL ensures that all application buffers are free to be reused.
 * A mode of HIO_FLUSH_MODE_COMPLETE ensures that the data has been flushed to the
 * current active data root.
 *
 * This function will return an error code if any write on the dataset can not
 * complete.
 */
int hio_flush_all (hio_dataset_t dataset, hio_flush_mode_t mode);

/**
 * @ingroup blocking
 * @brief Start a blocking contiguous read from an hio element
 *
 * @param[in]  element      hio element handle
 * @param[in]  offset       offset to read from
 * @param[in]  reserved0    reserved for future use (pass 0)
 * @param[in]  ptr          buffer to read data into
 * @param[in]  count        number of elements to read
 * @param[in]  size         size of each element
 *
 * @returns the number of bytes read if the read was successful
 *
 * This function reads contiguous data from the element specified in
 * {element} to the contiguous buffer specified in {ptr}. The call
 * returns when the buffer pointed to by {ptr} contains the requested
 * data or the read failed.
 */
ssize_t hio_read (hio_element_t element, off_t offset, unsigned long reserved0, void *ptr,
                  size_t count, size_t size);

/**
 * @ingroup nonblocking
 * @brief Start a non-blocking contiguous read from an hio element
 *
 * @param[in]  element      hio element handle
 * @param[out] request      new hio request (may be NULL)
 * @param[in]  offset       offset to read from
 * @param[in]  reserved0    reserved for future use (pass 0)
 * @param[in]  ptr          buffer to read data into
 * @param[in]  count        number of elements to read
 * @param[in]  size         size of each element
 *
 * @returns HIO success if the read has successfully been scheduled or
 *          completed
 *
 * This function schedules a non-blocking read of contiguous data from the
 * element specified in {element} to the contiguous buffer specified in
 * {ptr}. The call returns immediately even if the read has not completed. If
 * the request is not yet complete and a non-NULL value is specified in
 * {request} this function will return an hio request in {request}. The
 * application is requires to call one of hio_test(), hio_wait(), or
 * hio_join() on the returned request to ensure all resources are freed. If a
 * NULL value is specified in {request} any error occurring during the read
 * will be reported by hio_complete().
 * The hio implementation is free to return HIO_REQUEST_NULL in {request} if the
 * read is complete. In the context of reads a request is complete when the buffer
 * specified by {ptr} contains the requested data or the request failed.
 */
int hio_read_nb (hio_element_t element, hio_request_t *request, off_t offset,
                 unsigned long reserved0, void *ptr, size_t count, size_t size);

/**
 * @ingroup blocking
 * @brief Start a blocking strided read from an hio element
 *
 * @param[in]  element     hio element handle
 * @param[in]  offset      offset to read from
 * @param[in]  reserved0   reserved for future use (pass 0)
 * @param[in]  ptr         buffer to read data into
 * @param[in]  count       number of elements to read
 * @param[in]  size        size of each element
 * @param[in]  stride      stride between each element
 *
 * @returns the number of bytes read if the read was successful
 *
 * This function reads contiguous data from the element specified in
 * {element} to the strided buffer specified by {ptr}, {size},
 * and {stride}. This call returns when the buffer pointed to by {ptr}
 * contains the requested data or the read failed.
 */
int hio_read_strided (hio_element_t element, off_t offset, unsigned long reserved0,
                      void *ptr, size_t count, size_t size, size_t stride);

/**
 * @ingroup nonblocking
 * @brief Start a non-blocking strided read from an hio element
 *
 * @param[in]  element     hio element handle
 * @param[out] request     new hio request (may be NULL)
 * @param[in]  offset      offset to read from
 * @param[in]  reserved0   reserved for future use (pass 0)
 * @param[in]  ptr         buffer to read data into
 * @param[in]  count       number of elements to read
 * @param[in]  size        size of each element
 * @param[in]  stride      stride between each element
 *
 * @returns HIO success if the read has successfully been scheduled or
 *          completed
 *
 * This function schedules a non-blocking read of contiguous data from the
 * element specified in {element} to the strided buffer specified by {ptr},
 * {size}, and {stride}. The call returns immediately even if the read has not completed. If
 * the request is not yet complete and a non-NULL value is specified in
 * {request} this function will return an hio request in {request}. The
 * application is requires to call one of hio_test(), hio_wait(), or
 * hio_join() on the returned request to ensure all resources are freed. If a
 * NULL value is specified in {request} any error occurring during the read
 * will be reported by hio_complete().
 * The hio implementation is free to return HIO_REQUEST_NULL in {request} if the
 * read is complete. In the context of reads a request is complete when the buffer
 * specified by {ptr} contains the requested data or the request failed.
 */
int hio_read_strided_nb (hio_element_t element, hio_request_t *request, off_t offset,
                         unsigned long reserved0, void *ptr, size_t count, size_t size, size_t stride);

/**
 * @ingroup nonblocking
 * @brief Complete all outstanding read operations on an hio element.
 *
 * @param[in] element hio element handle
 *
 * @returns HIO_SUCCESS if all pending reads completed successfully
 * @returns the error code of the first failed read
 *
 * This function completes all outstanding reads on the element specified in
 * {element}. This function will return an error code if any read on the
 * dataset did not complete. Note: A short read is considered an error.
 */
int hio_complete (hio_element_t element);

/**
 * @ingroup API
 * @brief Join multiple requests into a single request object
 * 
 * @param[in,out] requests hio requests
 * @param[in]     count    length of {requests}
 * @param[out]    request  combined request
 *
 * This function combines the hio requests specified by {requests} and {count} and
 * combines them into a single hio request. The new request is complete when all
 * the joined requests are complete. The special value HIO_REQUEST_NULL stored
 * at any entry in {requests} is ignored. If every entry in the {requests} array
 * is HIO_REQUEST_NULL then HIO_REQUEST_NULL is returned in {request}.
 */
int hio_request_join (hio_request_t *requests, int count, hio_request_t *request);

/**
 * @ingroup API
 * @brief Test for completion of an I/O request
 *
 * @param[in,out] request  hio I/O request
 * @param[out]   complete flag indicating whether the request is complete.
 *
 * This function checks for the completion of an IO request. If the hio
 * request is complete the number of bytes transferred is stored in
 * {bytes_transferred} and the special value HIO_REQUEST_NULL is stored in
 * {request}. Additionally, if the request is complete this function releases
 * the resources associated with the hio request. Calling this function with
 * {request} pointing to the special value HIO_REQUEST_NULL sets
 * {bytes_transferred} to 0 and {complete} to true and returns immediately.
 */
int hio_test (hio_request_t *request, ssize_t *bytes_transferred,
              bool *complete);

/**
 * @ingroup API
 * @brief Wait for completion of an I/O request
 *
 * @param[in,out] request            hio IO request
 * @param[out]    bytes_transferred  number of bytes read/written
 *
 * This function waits for the completion of an IO request. On completion
 * of the hio request this function stores the number of bytes transferred
 * to {bytes_transferred} and sets the value pointed to by {request} to
 * HIO_REQUEST_NULL. Before returning this function also releases the
 * resources associated with the hio request. Calling this function with
 * {request} pointing to the special value HIO_REQUEST_NULL sets
 * {bytes_transferred} to 0 and returns immediately.
 */
int hio_wait (hio_request_t *request, ssize_t *bytes_transferred);

/**
 * @ingroup API
 * @brief Get recommendation on if a checkpoint should be written
 *
 * @param[in]  ctx  hio context
 * @param[out] hint Recommendation on checkpoint
 *
 * This function determines if it is optimal to checkpoint at
 * this time. This function will take into account the current data root and
 * expected checkpoint size. It will also query the runtime, system
 * configuration, or past I/O activity on the context to come up with the
 * recommendation. If the recommendation is returned in {hint} is HIO_SCP_NOT_NOW
 * the caller should not attempt to write a checkpoint at this time. If {hint}
 * is HIO_SCP_MUST_CHECKPOINT it is strongly recommended that the application
 * checkpoint now.
 */
void hio_should_checkpoint (hio_context_t ctx, int *hint);

/**
 * @ingroup configuration
 * @brief Set the value of an hio configuration variable
 *
 * @param[in] object    hio object to configure
 * @param[in] variable  variable to set
 * @param[in] value     new value for this variable
 *
 * This function sets the value of the given variable within the hio object
 * specified in {object}. {object} must be be one of NULL (global), an hio
 * context, or an hio dataset. The value is expected to be a string
 * representation that matches the variable's type as returned by
 * hio_config_list_variables(). Dataset specific values can be set on a
 * a context and will apply to all datasets opened in that context.
 */
int hio_config_set_value (hio_object_t object, char *variable, char *value);

/**
 * @ingroup configuration
 * @brief Get the string representation of the value of an hio
 * configuration variable.
 *
 * @param[in]  object    hio object to get the configuration from
 * @param[in]  variable  variable to get the value of
 * @param[out] value     string representation of the value of {variable}
 *
 * This function gets the string value of the given variable within the hio
 * object specified in {object}. {object} must be one of NULL (global), an hio
 * context, or an hio dataset. On success the string representation of the
 * value is stored in a newly allocated pointer and returned in {value}. It is
 * the responsibility of the caller to free {value}.
 */
int hio_config_get_value (hio_object_t object, char *variable, char **value);

/**
 * @ingroup configuration
 * @brief Get the number of configuration variables
 *
 * @param[in]  object  hio object
 * @param[out] count   the number of configuration variables
 *
 * @returns hio_return_t
 */
int hio_config_get_count (hio_object_t object, int *count);

/**
 * @ingroup configuration
 * @brief Retrieve information about an hio configuration variable
 *
 * @param[in]    object     hio object
 * @param[in]    index      configuration variable index
 * @param[out]   name       name of the configuration variable
 * @param[out]   type       type of the configuration variable
 * @param[out]   read_only  true if the variable is read-only, false otherwise
 *
 * @returns hio_return_t
 *
 * This function gets info about the requested variable including its name, type,
 * and whether the variable is read-only (informational). Subsequent calls to this
 * function with the same index will always return the same values. On successful
 * return {name} will contain the name of the variable, {type} will contain its type,
 * and {read_only} will indicate whether the variable is read-only. It is the
 * responsibility of the caller to free the pointer returned in {name}.
 *
 * The value specified in {index} should be a number between 0 and hio_config_get_count().
 * If a non-existent index is specified an error is returned.
 */
int hio_config_var_get_info (hio_object_t object, int index, char **name, hio_config_type_t *type,
                             bool *read_only);

/**
 * @ingroup performance
 * @brief Get the number of performance variables for an hio object
 *
 * @param[in]  hio_object  hio object
 * @param[out] count       the number of performance variables
 *
 * @return HIO_SUCCESS on success
 */
int hio_perf_var_get_count (hio_object_t hio_object, int *count);

/**
 * @ingroup performance
 * @brief Retrieve information about an hio configuration variable
 *
 * @param[in]    object     hio object
 * @param[in]    index      performance variable index
 * @param[out]   name       name of the performance variable
 * @param[out]   type       type of the performance variable
 *
 * @returns hio_return_t
 *
 * This function gets info about the requested performance variable including its name
 * and type, Subsequent calls to this function with the same {index} and {object} will
 * always return the same values. On successful return {name} will contain the name of
 * the performance variable and {type} will contain its type. It is the responsibility
 * of the caller to free the pointer returned in {name}.
 *
 * The value specified in {index} should be a number between 0 and hio_perf_get_count().
 * If a non-existent index is specified an error is returned.
 */
int hio_perf_var_get_info (hio_object_t hio_object, int index, char **name, hio_config_type_t *type);

/**
 * @ingroup performance
 * @brief Get the value of a performance variable
 *
 * @param[in]  object     hio object to read the variable from
 * @param[in]  variable   variable to get the value of
 * @param[out] value      current value of the variable
 * @param[in]  value_len  the length of the buffer specified in {value}
 *
 * This function gets the value of the given variable within the hio
 * object specified in {object}. {object} must be one of NULL (global), an hio
 * context, or an hio dataset. On success the value of the performance variable
 * is stored in the buffer specified in {value} according to it's type.
 */
int hio_perf_var_get_value (hio_object_t object, char *variable, void *value, size_t value_len);

#endif /* !defined(HIO_H) */