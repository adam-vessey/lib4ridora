<?php
/**
 * @file
 * Theme hooks.
 */

/**
 * Implements hook_preprocess_lib4ridora_pdf_materials().
 */
function template_preprocess_lib4ridora_pdf_materials(&$variables) {
  $object = $variables['object'];
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  $pdfs = lib4ridora_retrieve_filtered_pdfs($object);
  $variables['pdf_listing'] = array();
  foreach ($pdfs as $pdf) {
    $variables['pdf_listing'][$pdf->id] = array(
      '#theme' => 'lib4ridora_pdf_link',
      '#datastream' => $pdf,
      '#statement' => $variables['statement'],
    );
  }
}

/**
 * Implements hook_process_lib4ridora_pdf_materials().
 */
function template_process_lib4ridora_pdf_materials(&$variables) {
  $variables['pdf_listing'] = drupal_render($variables['pdf_listing']);
}

/**
 * Implements hook_preprocess_lib4ridora_pdf_link().
 */
function template_preprocess_lib4ridora_pdf_link(&$variables) {
  $ds = $variables['datastream'];
  $values = lib4ridora_get_embargo_info($ds);
  $variables['pdf_link'] = $values;
  $access = islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $variables['datastream']);
  $variables['availability_classes'] = array(
    'availability-text',
  );
  if ($access) {
    $variables['span_classes_array'][] = 'fa fa-unlock-alt';
  }
  else {
    $variables['span_classes_array'][] = 'fa fa-lock';
  }
}

/**
 * Implements hook_process_lib4ridora_pdf_link().
 */
function template_process_lib4ridora_pdf_link(&$variables) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  $values = $variables['pdf_link'];
  if ($variables['statement']) {
    $availability = $values['availability'];
    if ($availability == 'intranet') {
      $availability_text = t('Intranet Only');
    }
    elseif ($availability == 'date') {
      // Need to get a huamn readable month.
      $date = new DateTime();
      $date->createFromFormat('!m', $values['embargo_date']['month']);
      $availability_text = t('Intranet Only until @day @month @year', array(
        '@day' => $values['embargo_date']['day'],
        '@month' => $date->format('F'),
        '@year' => $values['embargo_date']['year'],
      ));
    }
    elseif ($availability == 'private') {
      $availability_text = t('Private');
    }
    else {
      $availability_text = t('Open Access');
    }
    $variables['availability'] = $availability_text;

  }
  $human_readable = lib4ridora_multi_pdf_get_options();
  $human_version = $human_readable['document_version'];
  $variables['title_link'] = array(
    'title' => $human_version[$values['document_version']],
    'url' => url("islandora/object/{$variables['datastream']->parent->id}/datastream/{$variables['datastream']->id}/view"),
  );
}

/**
 * Implements hook_preprocess_lib4ridora_citation_solr_results().
 */
function template_preprocess_lib4ridora_citation_solr_results(&$variables) {
  foreach ($variables['citations'] as &$citation) {
    $citation['pdf_listing'] = array(
      '#theme' => 'lib4ridora_pdf_materials',
      '#object' => islandora_object_load($citation['pid']),
    );
  }
}

/**
 * Implements hook_process_lib4ridora_citation_solr_results().
 */
function template_process_lib4ridora_citation_solr_results(&$variables) {
  foreach ($variables['citations'] as &$citation) {
    $citation['pdf_listing'] = drupal_render($citation['pdf_listing']);
  }
}

/**
 * Implements hook_preprocess_lib4ridora_islandora_solr_metadata_injected_author_info().
 */
function template_preprocess_lib4ridora_islandora_solr_metadata_injected_author_info(&$variables) {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $variables['author_href'] = array(
    'islandora/search',
    array(
      'query' => array(
        'f' => array(
          format_string('!field:"!value"', array(
            '!field' => islandora_solr_lesser_escape('mods_name_personal_author_authority_marcrelator_nameIdentifier_authorId_ms'),
            '!value' => $variables['info']['author']['id'],
          )),
        ),
      ),
    ),
  );
  $variables['org_href'] = array(
    'islandora/search',
    array(
      'query' => array(
        'f' => array(
          format_string('!field:"!value"', array(
            '!field' => islandora_solr_lesser_escape('mods_name_personal_author_authority_marcrelator_nameIdentifier_organizational unit id_ms'),
            '!value' => $variables['info']['org']['id'],
          )),
        ),
      ),
    ),
  );
}

/**
 * Implements hook_process_lib4ridora_islandora_solr_metadata_injected_author_info().
 */
function template_process_lib4ridora_islandora_solr_metadata_injected_author_info(&$variables) {
  $variables['author_href'] = call_user_func_array('url', $variables['author_href']);
  $variables['org_href'] = call_user_func_array('url', $variables['org_href']);
}
