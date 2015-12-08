<?php
/**
 * @file
 * Default template for lib4ridora-islandora-solr-metadata-injected-author-info.
 *
 * Somewhat gross, due to the default nl2br-type stuffs.
 *
 * Available variables:
 * - $author_href: URL for the author.
 * - $org_href: URL for the author's org.
 * - $info: Associative array containing:
 *   - author
 *     - name
 *     - id
 *   - org
 *     - name
 *     - id
 */
?>
<span class="<?php print $classes;?>"><a class="name" href="<?php print check_plain($author_href);?>"><?php print $info['author']['name'];?></a><?php
  if ($org_href && $info['org']['name']):?> (<a class="org" href="<?php print check_plain($org_href); ?>"><?php print $info['org']['name']; ?></a>)<?php
  endif;?></span>
