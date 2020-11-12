<template>
  <div class="card-deck" :class="{ [variant]: variant, [direction]: direction }">
    <slot></slot>
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
  props: {
    variant: {
      type: String,
      default: ""
    },
    direction: {
        type: String,
        default: ""
    }
  },
});
</script>