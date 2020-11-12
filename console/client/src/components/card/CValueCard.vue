<template>
  <div class="card value-card" :class="{[variant]: variant}">
    <div class="value">{{ data }}</div>
    <div class="description">
      <slot name="description"> </slot>
    </div>
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
  props: ["data", "variant"]

});
</script>