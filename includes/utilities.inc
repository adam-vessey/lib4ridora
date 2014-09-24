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
    $factor_datastream->mimetype = 'application/json';
    $factors = array();
  }

  $factors[$year] = $factor;
  $factor_datastream->content = json_encode($factors);

  // Doesn't do anything if ingested already.
  $journal->ingestDatastream($factor_datastream);
}
