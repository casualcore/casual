<template>
  <div>
      <h3 class="float">{{executable.alias}}</h3>
      <c-value-card :data="'alias'">
          <template v-slot:description>{{executable.alias}}</template>
      </c-value-card>
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
import {CValueCard} from "@/components/card";
import {ServerExtended}from "@/models";
import {ApiService} from "@/services";
export default defineComponent({
  name: "Executable",
  components: {
      CValueCard
  },
  data() {
    return {
      alias: this.$route.query.alias as string,
      executable: new ServerExtended()
    };
  },
  async created() {
    await this.getExecutable();
  },
  methods: {
    getExecutable: async function(): Promise<void> {
      this.executable = await ApiService.getExecutableByAlias(this.alias);
    }
  }
});
</script>
