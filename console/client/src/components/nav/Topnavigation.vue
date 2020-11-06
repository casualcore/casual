<template>
  <div id="topnav" :class="{ collapsed: collapsed }">
    <div id="console-head-line">
      <b>Casual Console</b>
      <p id="version">Casual version: {{ version }}</p>
    </div>
    <div id="user-menu" @click="toggleMenu" :class="{ active: menuActive }">
      <div class="menu-text button button--text logout">Log out</div>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent, ref } from "vue";
export default defineComponent({
  name: "Topnavigation",
  props: ["collapsed"],
  data() {
    return {
      casualVersion: "0.0.1",
      menuActive: false
    };
  },
  computed: {
    version: function(): string {
      return this.casualVersion;
    }
  },
  methods: {
    toggleMenu: function(toggle: boolean): void {
      if (this.menuActive && !toggle) {
        this.menuActive = false;
      } else {
        this.menuActive = true;
        //this.refs['userMenu'].focus();
      }
    },
    run: function(): void {
      console.log("focus");
    }
  }
});
</script>
<style lang="scss" scoped>
#user-menu {
  position: relative;
  //background-color: #3a424a;
  //padding: auto 4px;

  ul {
    color: white;
    font-size: 14pt;
    margin-top: 10px;
    z-index: 10000;
    background-color: #3a424a;
    display: none;
    position: absolute;
    right: 0;
    text-align: right;
    width: 150px;
    padding: 0;

    box-shadow: 2px 2px 8px 1px rgba(0, 0, 0, 0.3);
    li {
      margin: 0;
      padding: 0;
      padding: 3px 15px;
      &:hover {
        background-color: #292e34;
        cursor: pointer;
      }
    }
  }

  &.active {
    ul {
      display: block;
    }
  }

  .menu-text {
    cursor: pointer;
  }
}
</style>
