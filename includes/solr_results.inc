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
        "prefix" => array(
          "Organizational Unit",
        ),
      ),
      'info:fedora/islandora:collectionCModel' => array(
        "dc.title",
        "prefix" => array(
          "Collection",
        ),
      ),
      'info:fedora/lib4ri:journalCModel' => array(
        "fgs_label_s",
        "prefix" => array(
          "Journal",
        ),
      ),
      'info:fedora/islandora:personCModel' => array(
        'MADS_given_mt',
        'MADS_family_mt',
        "prefix" => array(
          "Author",
        ),
      ),
    );

    // Initilize our array's and markup variables.
    $markup = "";
    $data = array(
      "key" => NULL,
      "fields" => NULL,
      "prefix" => "",
    );

    foreach ($cmodels as $key => $value) {
      $found = array_search($key, $object_result['content_models']);
      if (FALSE !== $found) {
        $data['key'] = $key;
        $data['fields'] = $cmodels[$key];
        $data['prefix'] = $cmodels[$key]['prefix'][0];
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
   * @param array $data
   *   Key value array of CModels and associated fields.
   *
   * @param array $object_results
   *   A result from the query processor.
   *
   * @return string
   *   Concatenated and space separated string of provided
   *   Fields.
   */
  protected function getMarkupForCModel($data, $object_results) {
    $markup = "";
    foreach ($data['fields'] as $key => $value) {
      if (is_string($value)) {
        $field = isset($object_results['solr_doc'][$value]) ? $object_results['solr_doc'][$value] : "";
        if (is_array($field)) {
          $markup .= implode(" ", $field) . " ";
        }
        else {
          $markup .= $field . " ";
        }
      }
    }
    return  t("@prefix", array("@prefix" => $data['prefix'])) . ": " . $markup;
  }

  /**
   * Displays elements of the current solr query.
   *
   * Displays current query and current filters. Includes a link to exclude the
   * query/filter.
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Rendered lists of the currently active query and/or filters.
   */
  public function currentQuery($islandora_solr_query) {
    module_load_include('inc', 'islandora', 'includes/utilities');

    $output = '';
    $path = current_path();
    $format = variable_get('islandora_solr_facet_date_format', 'Y');

    // Get user provided filter parameters.
    $fq = isset($islandora_solr_query->internalSolrParams['f']) ? $islandora_solr_query->internalSolrParams['f'] : array();
    // Parameters set in URL.
    $params = $islandora_solr_query->internalSolrParams;
    // Get query values.
    if (!in_array($islandora_solr_query->solrQuery, $islandora_solr_query->differentKindsOfNothing)) {
      // Get query value.
      $query_value = $islandora_solr_query->solrQuery;
      $query_list = array();

      // Remove link keeps all parameters (query gets removed instead).
      $query_minus = $params;

      // Remove query from path.
      $path_minus = implode('/', explode('/', $path, -1));

      // Set attributes variable for remove link.
      $attributes = array(
        'minus' => array(
          'attr' => array(),
          'path' => $path_minus,
          'query' => $query_minus,
        ),
      );
      $attr_minus =& $attributes['minus']['attr'];
      $attr_minus['title'] = t('Remove') . ' ' . $query_value;
      $attr_minus['class'] = array('remove-query');
      $attr_minus['rel'] = 'nofollow';
      $attr_minus['href'] = url($path_minus, array('query' => $query_minus));

      $hooks = islandora_build_hook_list(ISLANDORA_SOLR_FACET_BUCKET_CLASSES_HOOK_BASE);
      drupal_alter($hooks, $attributes, $islandora_solr_query);

      // XXX: We are not using l() because of active classes:
      // @see http://drupal.org/node/41595
      // Create link.
      $query_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> ' . check_plain($query_value);

      // Add wrap and list.
      $output .= '<div class="islandora-solr-query-wrap">';
      $output .= theme('item_list', array(
        'items' => $query_list,
        'title' => t('Query'),
        'type' => 'ul',
        'attributes' => array('class' => 'islandora-solr-query-list query-list'),
      ));
      $output .= '</div>';

    }

    // Get filter values.
    if (!empty($fq)) {
      // Gather the human readable labels of the available facets.
      $records = islandora_solr_get_fields("facet_fields", FALSE, FALSE);

      // Set list variables.
      $filter_list = array();
      foreach ($fq as $key => $filter) {
        // Check for exclude filter.
        if ($filter[0] == '-') {
          // Not equal sign.
          $symbol = '&ne;';
        }
        else {
          $symbol = '=';
        }
        $filter_string = $this->formatFilter($filter, $islandora_solr_query);
        // Pull out filter (for exclude link).
        $query_minus = array();
        $f_x['f'] = array_diff($params['f'], array($filter));
        $query_minus = array_merge($params, $f_x);
        // @todo Find a cleaner way to do this.
        // Resetting the filter keys' order.
        if ($query_minus['f']) {
          $query_minus['f'] = array_merge(array(), $query_minus['f']);
        }
        // Remove 'f' if empty.
        if (empty($query_minus['f'])) {
          unset($query_minus['f']);
        }
        // Set attributes variable for remove link.
        $attributes = array(
          'minus' => array(
            'attr' => array(),
            'path' => $path,
            'query' => $query_minus,
          ),
        );
        $attr_minus =& $attributes['minus']['attr'];
        $attr_minus['title'] = t('Remove') . ' ' . $filter;
        $attr_minus['class'] = array('remove-filter');
        $attr_minus['rel'] = 'nofollow';
        $attr_minus['href'] = url($path, array('query' => $query_minus));

        $hooks = islandora_build_hook_list(ISLANDORA_SOLR_FACET_BUCKET_CLASSES_HOOK_BASE);
        drupal_alter($hooks, $attributes, $islandora_solr_query);

        // Append the human readable labels to the front of
        // the facet removeal link, in the
        // 'current_query_facet_name' block.
        foreach ($records as $key => $value) {
          if (strpos($filter, $value['solr_field']) !== FALSE) {
            // XXX: We are not using l() because of active classes:
            // @see http://drupal.org/node/41595

            // The following differs from the default
            // islandora_query block currentQuery().
            $filter_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a>' . " {$value['solr_field_settings']['label']}: " . $symbol . " " . check_plain($filter_string);
          }
        }
      }

      // Return filter list.
      $output .= '<div class="islandora-solr-filter-wrap">';
      $output .= theme('item_list', array(
        'items' => $filter_list,
        'title' => t("Enabled Filters"),
        'type' => 'ul',
        'attributes' => array('class' => 'islandora-solr-filter-list filter-list booger'),
      ));
      $output .= '</div>';
    }
    return $output;
  }
}
