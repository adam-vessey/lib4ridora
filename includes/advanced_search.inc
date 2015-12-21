<?php

/**
 * @file
 * Utiilty functions for Lib4Ri's altered advanced search form.
 */

/**
 * Queries solr to get all organizational units.
 *
 * @return array
 *   Associative array of organizational units formatted for #options
 */
function lib4ridora_get_all_organizational_units() {
  $facet_fields = lib4ridora_parse_affiliation_fields();
  $facets = lib4ridora_facet_query($facet_fields);

  $org_units = array();

  // If there's results, construct an array where the keys are affiliations
  // as in Solr, and the values are formatted to be labels.
  foreach ($facets as $facet) {
    if (empty($facet)) {
      continue;
    }
    $keys = array_keys($facet);
    $values = array_map('ucwords', $keys);
    $org_units = array_merge($org_units, array_combine($keys, $values));
  }

  // Return 'all' alongside whatever else.
  return array("*" => t("All")) + $org_units;
}

/**
 * Pulls solr affiliation field names out of configuration.
 *
 * @return array
 *   Array of affilation field names.
 */
function lib4ridora_parse_affiliation_fields() {
  $comma_separated_list = variable_get('lib4ridora_solr_field_affiliations', 'mods_name_personal_affiliation_ms,mods_name_corporate_affiliation_ms,mods_name_conference_affiliation_ms');
  $exploded = explode(",", $trimmed);
  return array_map("lib4ridora_trim_and_solr_escape", $exploded);
}

/**
 * Trims and solr escapes a string.
 *
 * @param string $str
 *   String to trim and escape.
 *
 * @return string
 *   Trimmed and escaped string, suitable for a solr query.
 */
function lib4ridora_trim_and_solr_escape($str) {
  module_load_include("inc", "islandora_solr", "includes/utilities");
  return islandora_solr_lesser_escape(trim($str));
}

/**
 * Issues a facet query to solr for the provided fields.
 *
 * @return array $facet_fields
 *   Array of fields to generate facets.
 *
 * @return array
 *   Associative array where key is the facet field and the value is the count.
 */
function lib4ridora_facet_query(array $facet_fields) {
  $qp = new IslandoraSolrQueryProcessor();
  $query = '*:*';
  $qp->buildQuery($query);
  $qp->solrParams['fl'] = "PID";
  $qp->solrParams['facet.mincount'] = 1;
  $qp->solrParams['facet.field'] = $facet_fields;
  $qp->solrParams['facet'] = 'true';
  $qp->solrLimit = 1;
  $qp->executeQuery();

  return $qp->islandoraSolrResult['facet_counts']['facet_fields'];
}

/**
 * Builds a solr filter query based on publication or reporting year.
 *
 * @param array $form_state
 *   Form state
 *
 * @return string
 *   Solr filter query
 */
function lib4ridora_construct_year_filter($form_state) {
  // Get the values out of the form.
  $year_type = $form_state['values']['year']['select'];
  $to = empty($form_state['values']['year']['to']) ? "*" : $form_state['values']['year']['to'];
  $from = empty($form_state['values']['year']['from']) ? "*" : $form_state['values']['year']['from'];

  // If nothing was entered in the date fields, exit early.
  if (strcmp($to, "*") == 0 && strcmp($from, "*") == 0) {
    return "";
  }

  // Convert to proper format for solr.
  $to_date = new DateTime($to);
  $to = $to_date->format('Y-m-d\TH:i:s\Z');
  $from_date = new DateTime($from);
  $from = $from_date->format('Y-m-d\TH:i:s\Z');

  // Return fq string.
  module_load_include("inc", "islandora_solr", "includes/utilities");
  switch ($year_type) {
    case "publication year":
      $publication_year_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_publication_year', 'mods_originInfo_encoding_w3cdtf_keyDate_yes_dateIssued_dt'));
      return "$publication_year_field:[$from TO $to]";

    case "reporting year":
      $reporting_year_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_reporting_year', 'mods_originInfo_encoding_w3cdtf_type_reporting year_dateOther_dt'));
      return "$reporting_year_field:[$from TO $to]";

    default:
      return "";
  }
}

/**
 * Builds a solr filter query based on publication type.
 *
 * @param array $form_state
 *   Form state
 *
 * @return string
 *   Solr filter query
 */
function lib4ridora_construct_publication_type_filter($form_state) {
  module_load_include('inc', 'lib4ridora', 'includes/citation.subtypes');
  $genre_map = function ($subtype) {
    return $subtype['genres'];
  };
  // Collect the genres for the selected values.
  $genres_blocks = array_intersect_key(array_map($genre_map, lib4ridora_citation_subtypes()), drupal_map_assoc($form_state['values']['pub_type']));

  // Merge down to a flat array.
  $genres = array_reduce($genres_blocks, 'array_merge', array());

  if (!$genres) {
    // No genres; no filter needed.
    return '';
  }

  module_load_include("inc", "islandora_solr", "includes/utilities");
  $quote = function ($string) {
    return format_string('"!string"', array(
      '!string' => islandora_solr_facet_escape($string),
    ));
  };

  return format_string('!field:(!value)', array(
    '!field' => islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_publication_type', 'mods_genre_ms')),
    '!value' => implode(' OR ', array_map($quote, $genres)),
  ));
}

/**
 * Combines a field name / value pair into a solr query statement.
 *
 * @param string $field_name
 *   Solr field name
 * @param string $field_value
 *   Solr field value
 *
 * @return string
 *   Solr query statement
 */
function lib4ridora_construct_solr_statement($field_name, $field_value) {
  if (strcmp($field_value, "*") != 0) {
    $field_value = '"' . $field_value . '"';
  }
  return "$field_name:$field_value";
}

/**
 * Constructs a sequence of solr statements combined with OR.
 *
 * @param array $filters
 *   Array of solr query statements to combine into one big filter.
 *
 * @return string
 *   A single filter string that is the union of all provided filters.
 */
function lib4ridora_construct_filter_string_from_array($filters) {
  $total = count($filters);

  switch ($total) {
    case 0:
      return "";

    case 1:
      return reset($filters);

    default:
      return implode($filters, " OR ");
  }
}

/**
 * Builds a filter query based on organizational unit (e.g., affiliations).
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_organization_unit_filter(&$form_state) {
  // Get the organizational unit value from the dropdown.
  $org_unit = $form_state['values']['org_unit'];

  if (strcmp($org_unit, "*") == 0) {
    return "";
  }

  // List of all possible affiliation fields.
  // Parsed from configurations as a comma separated list.
  $affiliation_fields = lib4ridora_parse_affiliation_fields();

  $org_units = array_fill(0, count($affiliation_fields), $org_unit);

  $filters = array_map("lib4ridora_construct_solr_statement", $affiliation_fields, $org_units);

  return lib4ridora_construct_filter_string_from_array($filters);
}

/**
 * Builds a filter query based on peer review status.
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_peer_reviewed_filter(&$form_state) {
  module_load_include("inc", "islandora_solr", "includes/utilities");

  // Get field name out of configuration.
  $peer_review_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_peer_review', 'mods_note_peer review_ms'));

  $filters = array();
  if ($form_state['values']['peer_review']['yes']) {
    $filters[] = "$peer_review_field:yes";
  }
  if ($form_state['values']['peer_review']['no']) {
    $filters[] = "$peer_review_field:no";
  }
  if ($form_state['values']['peer_review']['not specified']) {
    $filters[] = "(*:* NOT $peer_review_field:*)";
  }

  return lib4ridora_construct_filter_string_from_array($filters);
}

/**
 * Builds a filter query based on a document's full text.
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_full_text_filter(&$form_state) {
  module_load_include("inc", "islandora_solr", "includes/utilities");

  // Get field names out of configuration.
  $document_version_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_document_version', 'RELS_INT_lib4ridora-multi-embargo-document_version_literal_ms'));
  $document_availability_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_document_availability', 'RELS_INT_lib4ridora-multi-embargo-availability_literal_ms'));

  $filters = array();

  if ($form_state['values']['full_text']['open access']) {
    $filters[] = "(fedora_datastreams_ms:PDF AND ($document_version_field:\"published version\" OR $document_version_field:\"accepted version\") AND (*:* NOT $document_availability_field:*))";
  }

  if ($form_state['values']['full_text']['restricted']) {
    $filters[] = "(fedora_datastreams_ms:PDF AND ($document_version_field:\"published version\" OR $document_version_field:\"accepted version\") AND $document_availability_field:*)";
  }

  if ($form_state['values']['full_text']['no full text']) {
    $filters[] = "(*:* NOT fedora_datastreams_ms:PDF)";
  }

  return lib4ridora_construct_filter_string_from_array($filters);
}
