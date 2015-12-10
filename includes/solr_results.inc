<?php

/**
 * @file
 * Contains methods to search solr and display results. Depends on
 * Apache_Solr_Php client.
 */

/**
 * Extension of IslandoraSolrResultsBookmark to create an alternative display
 * type. The purpose of overriding this for Lib4RI, is to remove the markup from
 * the returned $object_url_info, before being passed to bookmark's
 * generate_markup helper.
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
    // Define per content model and what fields should be rendered in
    // Addition to the Bookmark content. This is gross, but it is a
    // Custom display. These fields really should be configurable.
    $cmodels = array(
      'info:fedora/islandora:organizationCModel' => array(
        "fgs_label_s",
      ),
      'info:fedora/islandora:collectionCModel' => array(
        "dc.title",
      ),
      'info:fedora/lib4ri:journalCModel' => array(
        "fgs_label_s",
      ),
      'info:fedora/islandora:personCModel' => array(
        'MADS_given_mt',
        'MADS_family_mt'
      ),
    );

    // Initilize our array's and markup variables.
    $markup = "";
    $data = array(
      "key" => NULL,
      "fields" => NULL,
    );

    foreach ($cmodels as $key => $value) {
      $found = array_search($key, $object_result['content_models']);
      if (false !== $found) {
        $data['key'] = $key;
        $data['fields'] = $cmodels[$key];
        $markup .= $this->getMarkupForCModel($data, $object_result);
      }
    }

    $object_url_info = array(
      'path' => $object_result['object_url'],
      'params' => $object_result['object_url_params'],
      'markup' => "",
    );
    return array(
      'markup' => $markup . islandora_bookmark_generate_markup($object_result['PID'], $object_url_info),
    );
  }

  /**
   * Provide specific fields for given content models.
   *
   *   the cmodel in question.
   */
  protected function getMarkupForCModel($data, $object_results) {
    $markup = "";
    foreach ($data['fields']  as $key => $value) {
      $field = isset($object_results['solr_doc'][$value]) ? $object_results['solr_doc'][$value] : "";
      if (is_array($field)) {
        $markup .= implode(" ", $field) . " ";
      } else {
        $markup .= $field . " ";
      }
    }
    return $markup;
  }

}
