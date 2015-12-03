<?php

/**
 * @file
 * Contains methods to search solr and display results. Depends on
 * Apache_Solr_Php client.
 */

/**
 * Extention of IslandoraSolrResultsBookmark to create an alternative display type.
 */
class IslandoraSolrResultsLib4riBookmark extends IslandoraSolrResultsBookmark {
  /**
   * Constructor.
   */
  public function __construct() {
    parent::__construct();
  }

  /**
   * Build a row for inclusion in the tableselect.
   *
   * @param array $object_result
   *   A result from the query processor.
   *
   * @return array
   *   An associative array of cells, keyed according to the header provided in
   *   $this->getTableHeader().
   */
  protected function getTableRow($object_result) {
    $object_url_info = array(
      'path' => $object_result['object_url'],
      'params' => $object_result['object_url_params'],
      'markup' => "",
    );
    return array(
      'markup' => islandora_bookmark_generate_markup($object_result['PID'], $object_url_info),
    );
  }
}
