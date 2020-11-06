<template>
  <div>
    <div class="card">
      <c-table :items="groups" :key="groups.length"></c-table>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import { ApiService } from "@/services";
import { Group } from "@/models";
import CTable from "@/components/table/CTable.vue";
export default defineComponent({
  name: "Groups",
  components: {
    CTable
  },
  data() {
    return {
      groups: [] as Group[]
    };
  },
  created(): void {
    this.getAll();
  },
  methods: {
    getAll: async function(): Promise<void> {
      this.groups = await ApiService.getAllGroups();
    }
  }
});
</script>
