#pragma once

/* Preprocess an init script: transform bare `cmd args &` lines into
 * `usagi-reg cmd args` calls so sh can run the full script while UsagiInit
 * tracks services via the registration pipe.
 *
 * Returns a malloc'd path to a temporary file on success, or NULL on error.
 * The caller must unlink(3) and free(3) the returned string. */
char *preprocess_script(const char *input_path);
