<template>
  <div>
    <div class="card">
      <c-table :items="texecutables" :key="texecutables.length"></c-table>
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
      executables: [] as Server[]
    };
  },
  async created() {
    await this.getAll();
  },
  computed: {
    texecutables: function(): Server[] {
      return this.executables;
    }
  },
  methods: {
    getAll: async function(): Promise<void> {
      this.executables = await ApiService.getAllExecutables();
    }
  }
});
</script>
