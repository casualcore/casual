<template>
  <div class="casual-table">
    <table>
      <slot></slot>
    </table>
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent, PropType } from "vue";

export default defineComponent({
  name: "CTable"
});
</script>
