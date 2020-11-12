<template>
  <div class="card">
    {{ name }}
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
  name: "Group",
  data() {
    return {
      name: this.$route.query.name
    };
  }
});
</script>
