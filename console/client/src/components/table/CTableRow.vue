<template>
  <tr class="casual-table-item">
    <slot></slot>
  </tr>
</template>
<script lang="ts">
import Vue, { defineComponent, PropType } from "vue";
export default defineComponent({
  name: "CTableRow"
});
</script>
