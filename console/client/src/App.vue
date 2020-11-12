<template>
  <div id="casual">
    <div id="casual-content-wrapper" :class="{ collapsed: collapsed }">
      <div id="casual-content">
        <router-view />
      </div>
    </div>
    <navigation
      @collapse="collapse"
      :collapsed="collapsed"
      class="casual-navigation"
    ></navigation>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import Navigation from "@/components/nav/Navigation.vue";
export default defineComponent({
  components: {
    Navigation
  },
  data() {
    return {
      collapsed: false
    };
  },
  methods: {
    collapse: function(): void {
      console.log("coll");
      this.collapsed = !this.collapsed;
    }
  }
});
</script>
<style lang="scss">
@import "assets/scss/casual";
</style>
