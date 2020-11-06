<template>
  <div id="sidenav-nav" class="collapsable" :class="{ collapsed: collapsed }">
    <div class="logo">
      <router-link :to="{ name: 'Console' }"
        ><span id="logo-icon"></span
      ></router-link>
    </div>
    <ul id="sidenav-list">
      <sidenav-list-item itemlink="Groups" itemid="groups"
        >Groups</sidenav-list-item
      >
      <sidenav-list-item itemlink="Servers" itemid="servers"
        >Servers</sidenav-list-item
      >
      <sidenav-list-item itemlink="Executables" itemid="executables"
        >Executables</sidenav-list-item
      >
      <sidenav-list-item itemlink="Services" itemid="services"
        >Services</sidenav-list-item
      >
      <sidenav-list-item itemlink="Gateways" itemid="gateways"
        >Gateways</sidenav-list-item
      >
      <sidenav-list-item itemlink="Transactions" itemid="transactions"
        >Transactions</sidenav-list-item
      >
    </ul>

    <button class="button collapse " @click="collapse">
      <span
        class="collapse-icon collapsable"
        :class="{ collapsed: collapsed }"
        id="collapse-icon"
      ></span>
    </button>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import SidenavListItem from "./SidenavListItem.vue";
export default defineComponent({
  name: "Sidenavigation",
  components: { SidenavListItem },
  props: ["collapsed"],
  methods: {
    collapse: function(): void {
      this.$emit("collapse");
    }
  }
});
</script>
