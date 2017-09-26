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
      'aliases' => array('lpp'),
      'description' => 'Purges objects leaving a smaller subset for dev purposes.',
      'drupal dependencies' => array(
        'islandora',
        'lib4ridora',
      ),
      'options' => array(
        'namespace' => array(
          'description' => 'The namespace of PIDs you wish to purge.',
          'required' => TRUE,
        ),
        'genre' => array(
          'description' => 'The MODS/genre of the objects you wish to purge.',
          'required' => TRUE,
        ),
        'percentage_purge' => array(
          'description' => 'The percentage of the collection you wish to purge.',
          'required' => TRUE,
        ),
        'content_model' => array(
          'description' => 'The content model of the objects you wish to purge.',
          'required' => TRUE,
        ),
        'collection_pid' => array(
          'description' => 'The PID of the collection you wish to purge objects from.',
          'required' => TRUE,
        ),
        'ignore_multiple_pdf' => array(
          'description' => 'Flag to ignore objects with multiple PDFs.',
          'required' => FALSE,
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
 * Partially purge objects, leaving a smaller subset.
 */
function drush_lib4ridora_partial_purge() {
  $namespace = drush_get_option('namespace');
  $genre = drush_get_option('genre');
  $percentage_purge = drush_get_option('percentage_purge');
  $content_model = drush_get_option('content_model');
  $collection_pid = drush_get_option('collection_pid');
  $ignore_multiple_pdf = drush_get_option('ignore_multiple_pdf', FALSE);

  $user = user_load(1);
  $tuque = islandora_get_tuque_connection($user);
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery('PID:' . $namespace . '\:*');
  $qp->solrParams['fl'] = 'PID';
  $qp->solrParams['fq'] = array(
    'RELS_EXT_hasModel_uri_ms:"info:fedora/' . $content_model . '"',
    'RELS_EXT_isMemberOfCollection_uri_ms:"info:fedora/' . $collection_pid . '"',
    'mods_genre_ms:"' . $genre . '"',
  );
  if ($ignore_multiple_pdf) {

    // Lib4ri appends a number to the id of PDF datastreams beyond the first.
    // look for a datastream with the pattern PDF + any other character to
    // determine multiple PDFs.
    $qp->solrParams['fq'][] = format_string('-!field:/!prefix[0-9A-Za-z]+/', array(
      '!field' => 'fedora_datastreams_ms',
      '!prefix' => variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF'),
    ));
  }
  $qp->solrParams['facet'] = 'false';
  $qp->executeQuery(TRUE, FALSE);
  $results = $qp->islandoraSolrResult['response']['objects'];
  $has_children = function ($pid) use ($tuque) {
    $query = <<<EOQ
ASK {
  ?subject <fedora-model:hasModel> <info:fedora/fedora-system:FedoraObject-3.0> ;
           ?pred <info:fedora/$pid> .
}
EOQ;

    $child_results = $tuque->repository->ri->sparqlQuery($query);
    return $child_results[0]['k0']['value'] == 'true';
  };

  // Randomize the order of returned pids.
  shuffle($results);
  $index = 1;
  foreach ($results as $result) {
    $pid = $result['solr_doc']['PID'];
    // If we are dealing with a content model which could have children,
    // check to make sure there are no children of this object.
    if ($has_children($pid)) {
      continue;
    }
    try {
      $tuque->repository->purgeObject($pid);
      echo($pid . "\r\n");
      $index++;
    }
    catch (RepositoryException $e) {
      watchdog_exception('lib4ridora_partial_purge', $e);
      continue;
    }

    // Break when we've hit the percentage to keep.
    if (($index / (count($results)) * 100) >= $percentage_purge) {
      break;
    }
  }
}
