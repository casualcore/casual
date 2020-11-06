<template>
  <li class="sidenav-list-item">
    <router-link :to="{ name: itemlink }">
      <div class="side-nav-icon" :id="itemid.toLowerCase()"></div>
      <span> <slot></slot> </span>
    </router-link>
  </li>
</template>
<script lang="ts">
import { defineComponent } from "vue";
export default defineComponent({
  props: {
    itemlink: {
      type: String,
      default: "#"
    },
    itemid: {
      type: String,
      default: "groups"
    }
  }
});
</script>
