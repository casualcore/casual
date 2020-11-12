<template>
  <thead>
    <tr class="casual-table-head">
      <th v-for="item in headitems" :key="item">{{ item }}</th>
    </tr>
  </thead>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
  name: "CHead",
  props: ["headitems"]
});
</script>
