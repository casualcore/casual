<template>
  <td>
    <template v-if="route">
      <router-link class="casual-link" :to="{ name: route, query: query }"
        ><slot></slot
      ></router-link>
    </template>
    <template v-else><slot></slot></template>
  </td>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
  props: ["route", "query"]
});
</script>
