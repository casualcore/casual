<template>
  <li class="sidenav-list-item">
    <router-link
      :to="{ name: itemlink }"
      :class="{ 'router-link-active': subIsActive(itemlink) }"
    >
      <div class="side-nav-icon" :id="itemid.toLowerCase()"></div>
      <span> <slot></slot> </span>
    </router-link>
  </li>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import router from "@/router/index";
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
  },
  methods: {
    subIsActive: function(input: string): boolean {
      const paths = Array.isArray(input) ? input : [input];
      return paths.some(path => {
        {
          for (const r of router.getRoutes()) {
            if (r.name === path) {
              const route = r.path;
              return this.$route.path.indexOf(route) === 0;
            }
          }
        }
      });
    }
  }
});
</script>
