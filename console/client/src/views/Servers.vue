<template>
  <div>
    <div class="card">
      <c-table :items="tservers" :key="tservers.length"></c-table>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import { Server } from "@/models";
import { ApiService } from "@/services";
import CTable from "@/components/table/CTable.vue";
export default defineComponent({
  name: "Servers",
  components: {
    CTable
  },
  data() {
    return {
      servers: [] as Server[]
    };
  },
  async created() {
    await this.getAll();
  },
  computed: {
    tservers: function(): Server[] {
      return this.servers;
    }
  },
  methods: {
    getAll: async function(): Promise<void> {
      this.servers = await ApiService.getAllServers();
    }
  }
});
</script>
