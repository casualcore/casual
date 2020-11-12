<template>
  <select name="nbritems" id="nbritems" v-model="selected">
    <option v-for="n in nbr" :key="n">{{
      n
    }}</option>
  </select>
</template>
<script lang="ts">
import { defineComponent } from "vue";
export default defineComponent({
  name: "CSelectNbrItems",
  data() {
    return {
        nbr: [5, 10, 25, 50, 100, 200],
      selected: 5,
    };
  },
  watch: {
      selected: function(newVal) {
          this.$emit("change-input", newVal);
      }
  }
});
</script>