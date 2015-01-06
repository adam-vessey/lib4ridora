<?php

/**
 * @file
 * Utility functions for the module.
 */

/**
 * Retrieve the journal identified via an ISSSN or EISSN.
 *
 * @param string $identifier
 *   An ISSN or EISSN.
 *
 * @return mixed
 *   The object that is the journal or FALSE if none found.
 */
function lib4ridora_get_journal($identifier) {
  $journal = FALSE;
  $issn_field = variable_get('lib4ridora_issn_solr_field', 'mods_identifier_issn_s');
  $e_issn_field = variable_get('lib4ridora_issn_solr_field', 'mods_identifier_e-issn_s');
  $solr = new IslandoraSolrQueryProcessor();
  $solr->buildQuery("$issn_field:\"$identifier\" OR $e_issn_field:\"$identifier\"");
  $solr->solrLimit = 1;
  $solr->solrParams['fl'] = implode(
    ',',
    array(
      'PID',
      $issn_field,
      $e_issn_field,
    )
  );
  $solr->executeQuery(FALSE);
  if (isset($solr->islandoraSolrResult['response']['objects'][0])) {
    $journal = $solr->islandoraSolrResult['response']['objects'][0]['solr_doc']['PID'];
  }
  return islandora_object_load($journal);
}

/**
 * Get an impact factor.
 *
 * @param AbstractObject $journal
 *   The journal object to get the factor for.
 * @param string $year
 *   A four character string indicating the year of the factor.
 *
 * @return mixed
 *   The impact factor string or FALSE if not set.
 */
function lib4ridora_get_impact_factor(AbstractObject $journal, $year) {
  if (!isset($journal[LIB4RIDORA_FACTOR_DSID])) {
    return FALSE;
  }
  $impact_factors = json_decode($journal[LIB4RIDORA_FACTOR_DSID]->content, TRUE);
  return isset($impact_factors[$year]) ? $impact_factors[$year] : FALSE;
}

/**
 * Set the impact factor on a journal's year.
 *
 * @param AbstractObject $journal
 *   The journal object to set the factor for.
 * @param string $year
 *   A four character string indicating the year of the factor.
 * @param string $factor
 *   The factor to set.
 */
function lib4ridora_set_impact_factor(AbstractObject $journal, $year, $factor) {
  if (isset($journal[LIB4RIDORA_FACTOR_DSID])) {
    $factor_datastream = $journal[LIB4RIDORA_FACTOR_DSID];
    $factors = json_decode($factor_datastream->content, TRUE);
  }
  else {
    $factor_datastream = $journal->constructDatastream(LIB4RIDORA_FACTOR_DSID, 'M');
    $factor_datastream->label = 'Journal Impact Factors';
    $factor_datastream->mimetype = 'application/json';
    $factors = array();
  }

  $factors[$year] = $factor;
  $factor_datastream->content = json_encode($factors);

  // Doesn't do anything if ingested already.
  $journal->ingestDatastream($factor_datastream);
}

/**
 * Get factor with fallback to previous years.
 *
 * @param AbstractObject $object
 *   A journal object from which to attempt to fetch an impact factor.
 * @param array|string $issued_years
 *   Either a string or an array of strings, each numerical, representing years
 *   in which to test for impact factors. It was indicated that we should
 *   fallback to obtain factors for one year preceding, if a factor for the
 *   actual year could not be found.
 *   Allowing an array is for convenience, as silly things could happen with
 *   different date encodings we basing things on.
 *
 * @return array|bool
 *   If a factor could be found, an array containing the factor for the first
 *   year (in the order they were provided in $issued_years) and the year;
 *   otherwise, FALSE.
 */
function lib4ridora_get_factor_with_fallback(AbstractObject $object, $issued_years) {
  $issued_years = (array) $issued_years;
  $issued_years_with_fallbacks = $issued_years;
  foreach ($issued_years as $year) {
    // They wish to fallback to trying to use values from a preceding year.
    $issued_years_with_fallbacks[] = $year - 1;
  }
  $issued_years_with_fallbacks = array_unique($issued_years_with_fallbacks, SORT_NUMERIC);

  $factor = FALSE;
  foreach ($issued_years_with_fallbacks as $year) {
    $factor = lib4ridora_get_impact_factor($object, $year);
    if ($factor !== FALSE) {
      break;
    }
  }

  return $factor !== FALSE ? array($factor, $year) : FALSE;
}
