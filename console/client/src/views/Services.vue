<template>
  <div>
    <div class="card">
      <loading v-if="tservices.length == 0 && loading" />
      <c-table :items="tservices" :key="tservices.length"></c-table>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import CTable from "@/components/table/CTable.vue";
import { ApiService } from "@/services";
import { Service } from "@/models";
export default defineComponent({
  components: {
    CTable
  },
  data() {
    return {
      loading: false,
      services: [] as Service[]
    };
  },
  created() {
    this.loading = true;
    this.getAll();
    this.loading = false;
  },
  computed: {
    tservices: function(): Service[] {
      return this.services;
    }
  },
  methods: {
    getAll: async function(): Promise<void> {
      this.services = await ApiService.getAllServices();
    }
  }
});
</script>
