<?php

/**
 * @file
 * Implementation of Drush hooks.
 */

/**
 * Implements hook_drush_command().
 */
function lib4ridora_drush_command() {
  return array(
    'lib4ridora_lift_embargoes' => array(
      'description' => 'Lifts expired embargoes.',
      'drupal dependencies' => array(
        'islandora',
        'lib4ridora',
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    ),
    'lib4ridora_partial_purge' => array(
      'aliases' => 'lpp',
      'description' => 'Purges objects leaving a small subset for dev purposes.',
      'drupal dependencies' => array(
        'islandora',
        'lib4ridora',
      ),
      'options' => array(
        'namespace' => array(
          'description' => '',
          'required' => TRUE,
        ),
        'genre' => array(
          'description' => '',
          'required' => TRUE,
        ),
        'percentage_keep' => array(
          'description' => '',
          'required' => TRUE,
        ),
        'content_model' => array(
          'description' => '',
          'required' => TRUE,
        ),
        'collection_pid' => array(
          'description' => '',
          'required' => TRUE,
        ),
        'ignore_multiple_pdf' => array(
          'description' => '',
          'required;' => TRUE,
        ),
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    ),
  );
}

/**
 * Lift expired embargoes.
 */
function drush_lib4ridora_lift_embargoes() {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  module_load_include('inc', 'lib4ridora', 'includes/full_text');
  $now = gmdate("Y-m-d\TH:i:s\Z", time());
  $rels_ext_namespace = LIB4RIDORA_RELS_EXT_NAMESPACE;
  $rels_int_namespace = LIB4RIDORA_RELS_URI;
  // Look for any embargo's which have expired, and update their RELS-EXT as
  // appropriate.
  // We don't limited this to a date range, as the default behaviour
  // is to set the object's full text to 'Open Access', if any single PDF has an
  // expired embargo.
  $query = <<<EOT
PREFIX lib4ridora-rels-ext: <$rels_ext_namespace>
PREFIX lib4ridora-rels-int: <$rels_int_namespace>
SELECT ?object WHERE {
  ?object <fedora-model:state> <fedora-model:Active> ;
          <lib4ridora-rels-ext:fullText> "Restricted" ;
          <fedora-view:disseminates> ?ds .
  ?ds <fedora-view:mimeType> "application/pdf" ;
      <lib4ridora-rels-int:lib4ridora-multi-embargo-embargo_date> ?date .
  FILTER(?date < '$now'^^xsd:dateTime)
}
EOT;
  // Force the cron to run as user 1 so we don't get locked out of objects.
  $user = user_load(1);
  $repository = islandora_get_tuque_connection($user);
  $results = $repository->repository->ri->sparqlQuery($query);
  foreach ($results as $result) {
    $object = islandora_object_load($result['object']['value']);
    if ($object) {
      lib4ridora_update_full_text($object);
    }
  }
}

/**
 * Partially purge objects, leaving a small subset.
 */
function drush_lib4ridora_partial_purge($namespace, $genre, $content_model, $percentage_keep, $collection_pid, $ignore_multiple_pdf = FALSE) {
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery('PID:' . $namespace . '*');
  $qp->solrParams['fl'] = 'PID';
  $qp->solrParams['fq'] = array(
    'RELS_EXT_hasModel_uri_ms:"info:fedora/' . $content_model . '"',
    'RELS_EXT_isMemberOfCollection_uri_ms:"info:fedora/' . $collection_pid . '"',
    'mods_genre:"' . $genre . '"',
  );
  if ($ignore_multiple_pdf) {

    // Lib4ri appends a number to the id of PDF datastreams beyond the first.
    // look for a datastream with the pattern PDF + any other character to
    // determine multiple PDFs.
    array_push($qp->solrParams['fq'], 'fedora_datastreams_ms:"PDF/[0-9A-Za-z]+/"');
  }
  $qp->solrParams['facet'] = 'false';
  $qp->executeQuery(TRUE, FALSE);
  $results = $qp->islandoraSolrResult['response']['objects'];

  // Randomize the order of returned pids.
  shuffle($results);
  $index = 1;
  foreach ($results as $pid) {
    $result = $pid['solr_doc']['PID'];
    // If we are dealing with a content model which could have children,
    // check to make sure there are no children of this object.
    if (in_array($content_model, [
      "islandora:collectionCModel",
      "islandora:compoundCModel",
      "islandora:book",
    ])
    ) {
      $cqp = new IslandoraSolrQueryProcessor();
      $cqp->buildQuery('*:*');
      $cqp->solrParams['fq'] = array(
        '(RELS_EXT_isMemberOfCollection_uri_ms:"info:fedora/' . $result . '" OR RELS_EXT_isConstituentOf_uri_ms:"info:fedora/' . $result . '")',
      );
      $cqp->solrParams['facet'] = FALSE;
      $cqp->executeQuery(TRUE, FALSE);
      if (count($cqp->islandoraSolrResult['response']['objects']) > 0) {
        continue;
      }
    }
    $object = islandora_object_load($result);
    if ($object) {
      echo($result . "\r\n");
      // $object->repository->purgeObject($result);
    }

    // Break when we've hit the percentage to keep.
    if (($index / (count($results)) * 100) > $percentage_keep) {
      break;
    }
    $index++;
  }
}
