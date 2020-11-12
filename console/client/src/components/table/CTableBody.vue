<template>
  <tbody>
    <slot></slot>
  </tbody>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
  name: "CHead",
  props: ["headitems"]
});
</script>
