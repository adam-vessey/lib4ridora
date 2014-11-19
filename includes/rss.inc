<?php

/**
 * @file
 * Helper function(s) for generating the RSS feed.
 */

/**
 * Pare down MODS to what is wanted for RSS.
 *
 * @param DOMDocument $doc
 *   The MODS document to pare down.
 *
 * @return DOMDocument
 *   The pared MODS document.
 */
function lib4ridora_mods_for_rss(DOMDocument $doc) {
  $xslt_processor = new XSLTProcessor();
  $xslt_doc = new DOMDocument();
  $xslt_doc->load(drupal_get_path('module', 'lib4ridora') . '/xml/mods-pare.xslt');
  $xslt_processor->importStylesheet($xslt_doc);
  return $xslt_processor->transformToDoc($doc);
}
