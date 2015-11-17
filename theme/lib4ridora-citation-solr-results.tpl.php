<?php
/**
 * @file
 * Results view for Solr citations.
 *
 * Variables available:
 *
 * - $citations: an array of recent citations containing:
 *   'citation': The rendered citation from citeproc.
 *   'pid': The object PID.
 *   'pdfs': An array of PDFs to render. Each of these contain a 'version', a
 *   'dsid' and an 'id'.
 */
?>

<div id="lib4ridora-citation-solr-results">
  <?php foreach ($citations as $citation): ?>
  <div class="lib4ri-citation-solr-results-citation">
    <?php print $citation['citation']; ?>
    <span id="lib4ri-citation-detailed-record"><?php print l(t('Detailed Record'), "/islandora/object/{$citation['pid']}"); ?></span>
    <?php foreach ($citation['pdfs'] as $pdf): ?>
    <span id="<?php print $pdf['id']; ?>"><?php print l(ucwords($pdf['version']), "/islandora/object/{$citation['pid']}/datastream/{$pdf['dsid']}/view"); ?></span>
    <?php endforeach; ?>
  </div>
  <?php endforeach; ?>
</div>
