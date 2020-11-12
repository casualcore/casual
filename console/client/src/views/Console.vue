<template>
  <div class="card-deck">
    <div class="card" v-for="x in 10" :key="x">
      Casual console
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
export default defineComponent({});
</script>
