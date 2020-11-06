<template>
  <div id="console-navigation">
    <sidenavigation
      @collapse="collapse"
      :collapsed="collapsed"
    ></sidenavigation>
    <topnavigation :collapsed="collapsed"></topnavigation>
  </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import Sidenavigation from "./Sidenavigation.vue";
import Topnavigation from "./Topnavigation.vue";
export default defineComponent({
  name: "Navigation",
  props: ["collapsed"],
  components: {
    Sidenavigation,
    Topnavigation
  },
  mounted() {
    //TODO: Get collapsed status from cookie
  },
  methods: {
    collapse: function(): void {
      this.$emit("collapse");
    }
  }
});
</script>
