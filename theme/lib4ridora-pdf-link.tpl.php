<?php
/**
 * @file
 * Template file for the listing of an individual PDF link.
 */
?>

<div class="<?php print $classes; ?>">
<span class="<?php print implode(' ', $span_classes_array); ?>">
  <a href="<?php print $title_link['url']; ?>"><?php print $title_link['title']; ?></a>
</span>
    <?php if ($variables['availability']) :
      print $variables['availability'];
    endif; ?>
</div>
