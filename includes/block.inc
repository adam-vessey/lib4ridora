<?php

/**
 * @file
 * Block functionality for lib4ridora.
 */

/**
 * Gets a form element for an organization block content.
 *
 * @return array
 *   An array representing an organization block content form element.
 */
function lib4ridora_get_organizational_block_content() {
  $facets = lib4ridora_get_organization_facets();
  $field = variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms');
  $items = array();
  foreach ($facets as $facet => $count) {
    $items[] = array(
      'data' => l($facet, "islandora/search/$field:($facet)") . "<span class='label label-default'>($count)</span>",
      'class' => array('list-group-item'),
    );
  }
  return array(
    '#type' => 'item',
    '#theme' => 'item_list',
    '#items' => $items,
    '#attributes' => array(
      'class' => array('list-group'),
    ),
  );
}

/**
 * Gets a list of theme info for recently added citations.
 *
 * @return array
 *   Items to be themed into a list of recent citations, containing:
 *   - 'citation': A citation entry for the object,
 *   - 'pid': The PID of the object,
 *   - 'pdfs': an array of PDF datastreams on the object, containing a 'dsid',
 *     a 'version' and a 'class' for rendering.
 */
function lib4ridora_get_recent_additions() {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  module_load_include('inc', 'lib4ridora', 'includes/pdf_list');
  $style = citeproc_default_style();
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery('*:*');
  $qp->solrParams['fl'] = 'PID';
  $model_field = variable_get('islandora_solr_content_model_field', 'RELS_EXT_hasModel_uri_ms');
  $qp->solrParams['fq'][] = "$model_field:info\:fedora\/ir\:citationCModel";
  $qp->solrParams['sort'] = variable_get('lib4ridora_recent_additions_block_sort_parameter', 'fgs_createdDate_dt desc');
  $qp->solrLimit = 5;
  $qp->executeQuery(FALSE);
  $items = array();
  foreach ($qp->islandoraSolrResult['response']['objects'] as $item) {
    $object = islandora_object_load($item['PID']);
    if ($object) {
      // Render the citation.
      $mods = islandora_bibliography_get_mods($object->id);
      $entry = citeproc_bibliography_from_mods($style, $mods);
      $pdfs = lib4ridora_get_pdf_datastream_info($object);
      $items[] = array(
        'citation' => $entry,
        'pid' => $object->id,
        'pdfs' => $pdfs,
      );
    }
  }
  return $items;
}

/**
 * Helper to retrieve PDF information from an object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 * @param bool $availability
 *   Whether to grab the availability information as well.
 *
 * @return array
 *   An array containing information about the PDFs:
 *   -dsid (string): The datastream ID of the PDF.
 *   -version (string): The version type of the PDF.
 *   -id (string): An id to be used as a selector.
 */
function lib4ridora_get_pdf_datastream_info(AbstractObject $object, $availability = FALSE) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form.inc');
  $datastreams = iterator_to_array($object);
  $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');
  $pdfs = array();
  ksort($pdf_datastreams);
  foreach ($pdf_datastreams as $pdf_datastream) {
    $pdf_rels = $pdf_datastream->relationships->get();
    $parsed_rels = array();
    foreach ($pdf_rels as $values) {
      $parsed_rels[$values['predicate']['value']] = $values['object']['value'];
    }
    if (isset($parsed_rels['lib4ridora-multi-embargo-document_version'])) {
      $version = $parsed_rels['lib4ridora-multi-embargo-document_version'];
      $pdf_vals = array(
        'dsid' => $pdf_datastream->id,
        'version' => $version,
        'id' => 'lib4ri-citation-' . str_replace(' ', '-', $version),

      );
      if ($availability) {
        $pdf_vals += array(
          'availability' => array(
            'status' => 'public',
            'date' => FALSE,
            'statement' => t('Open Access'),
          ),
        );
        if (isset($parsed_rels['lib4ridora-multi-embargo-availability'])) {
          $pdf_vals['availability']['status'] = $parsed_rels['lib4ridora-multi-embargo-availability'];
          $pdf_vals['availability']['statement'] = ucwords($pdf_vals['availability']['status']);
          // Get the date.
          if ($pdf_vals['availability']['status'] == 'date') {
            $pdf_vals['availability']['date'] = $parsed_rels['lib4ridora-multi-embargo-embargo_date'];
            $pdf_vals['availability']['statement'] = t('Intranet only until @date', array(
              '@date' => format_date(strtotime($parsed_rels['lib4ridora-multi-embargo-embargo_date']), 'custom', 'd F Y'),
            ));
          }
          else {
            $pdf_vals['availability']['statement'] = t('Intranet only');
          }
        }
      }
      $pdfs[$pdf_datastream->id] = $pdf_vals;
    }
  }
  return $pdfs;
}

/**
 * Gets a form for configuring the organization facet block.
 *
 * @return array
 *   Form definition for items in the organization facet block configuration.
 */
function lib4ridora_organizational_block_configuration_form() {
  $form = array(
    'field' => array(
      '#type' => 'textfield',
      '#title' => t('Organization Solr Field'),
      '#description' => t('Solr field to use to search for organizations.'),
      '#default_value' => variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms'),
    ),
  );
  return $form;
}

/**
 * Gets a form for configuring the recent additions block.
 *
 * @return array
 *   Form definition for the recent additions block configuration.
 */
function lib4ridora_recent_additions_block_configuration_form() {
  $form = array(
    'sort_field' => array(
      '#type' => 'textfield',
      '#title' => t('Solr Sort Parameter'),
      '#description' => t('A parameter to use for sorting the Solr results.'),
      '#default_value' => variable_get('lib4ridora_recent_additions_block_sort_parameter', 'fgs_createdDate_dt desc'),
    ),
  );
  return $form;
}

/**
 * Returns the facet fields for a Solr facet search on organizations.
 *
 * @return array
 *   An associative array pairing faceted organization values with their count.
 */
function lib4ridora_get_organization_facets() {
  $org_field = variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms');
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery('*:*');
  $qp->solrParams['fl'] = $org_field;
  $qp->solrParams['facet.limit'] = 100000;
  $qp->solrParams['facet.mincount'] = 1;
  $qp->solrParams['facet.field'] = array();
  $qp->solrParams['facet.query'] = array('*');
  $qp->solrParams['facet.field'] = array($org_field);
  $qp->executeQuery(FALSE);
  return $qp->islandoraSolrResult['facet_counts']['facet_fields'][$org_field];
}

/**
 * Renders the markup for the full text block.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return array
 *   The markup to be rendered.
 */
function lib4ridora_get_fulltext_block_content($object) {
  $fulltext = array();
  $pdf_info = lib4ridora_get_pdf_datastream_info($object, TRUE);
  $version_classes = libfourri_theme_get_accepted_version_types();
  foreach ($pdf_info as $pdf) {
    $dsid = $pdf['dsid'];
    $version = $pdf['version'];
    $fulltext[$dsid] = array(
      'link' => array(
        '#theme' => 'link',
        '#text' => ucwords($version),
        '#path' => "islandora/object/{$object->id}/datastream/{$dsid}/view",
        '#options' => array(
          'attributes' => array('target' => '_blank'),
        ),
        '#prefix' => format_string('<div class="libfourri-fulltext"><span id="@id" class="@class">', array(
          '@id' => $pdf['id'],
          '@class' => $version_classes[$version],
        )),
        '#suffix' => '</span></div>',
      ),
    );
    // Add the availability.
    $fulltext[$dsid]['availability'] = array(
      '#markup' => $pdf['availability']['statement'],
    );
  }
  return $fulltext;
}

/**
 * Helper function to query to get the related items.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return bool|DOMNodeList
 *   A DOMNodeList of DOMElements to be rendered if present or FALSE if none
 *   exists.
 */
function lib4ridora_get_related_research_data(AbstractObject $object) {
  if (isset($object['MODS'])) {
    $dom = new DOMDocument();
    $dom->loadXML($object['MODS']->content);
    $xpath = new DOMXPath($dom);
    $xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
    $results = $xpath->query('//mods:relatedItem[@type="otherType"]/mods:identifier[@type="uri"]');
    return $results;
  }
  return FALSE;
}

/**
 * Renders the markup for related research data.
 *
 * @param DOMNodeList $results
 *   A DOMNodeList of DOMElements to be rendered.
 *
 * @return array
 *   An array representing the markup for the block
 */
function lib4ridora_get_related_research_data_block_content($results) {
  $markup = array();
  foreach ($results as $key => $result) {
    $url = $result->nodeValue;
    $markup["research_$key"] = array(
      '#theme' => 'link',
      '#text' => format_plural($results->length, 'View Research Data', 'View Research Data (!count)', array('!count' => $key + 1)),
      '#path' => $url,
      '#options' => array(
        'attributes' => array('target' => '_blank'),
      ),
      "#prefix" => '<div class="libfourri-research-data"><span>',
      '#suffix' => '</span></div>',
    );
  }
  return $markup;
}
