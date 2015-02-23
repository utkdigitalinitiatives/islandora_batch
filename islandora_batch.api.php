<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * A hook to allow for events directly after a islandora batch has finished.
 *
 * @param array $results
 *   The array of result data passed into the finish function. Note that when
 * processing a single set it contains the set id stored in ingest_set.
 */
function hook_islandora_batch_process_finished($results) {
}
