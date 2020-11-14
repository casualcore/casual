<template>
  <div>
    <div class="row">
      <div class="col-12">
        <div class="float">
          <div class="p-1">
            <div class="big">Services</div>
            <c-input class="right" placeholder="Filter" @changeInput="filter">
            </c-input>
          </div>
          <loading v-if="cservices.length == 0 && loading" />
          <c-table :key="filteredServices">
            <c-table-head :headitems="headitems" />
            <c-table-body>
              <c-table-row v-for="(service, index) in cservices" :key="index">
                <c-table-cell route="Service" :query="{ name: service.name }">{{
                  service.name
                }}</c-table-cell>
                <c-table-cell>{{ service.category }}</c-table-cell>
                <c-table-cell>{{ service.called }}</c-table-cell>
                <c-table-cell>{{ service.invoked.min }}</c-table-cell>
                <c-table-cell>{{ service.invoked.max }}</c-table-cell>
                <c-table-cell>{{ service.invoked.avg }}</c-table-cell>
                <c-table-cell>{{ service.invoked.total }}</c-table-cell>
                <c-table-cell>{{ service.pending.total }}</c-table-cell>
              </c-table-row>
            </c-table-body>
          </c-table>
        </div>
      </div>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import {
  CTable,
  CTableHead,
  CTableRow,
  CTableBody,
  CTableCell
} from "@/components/table";
import { CInput } from "@/components/input";
import { ApiService } from "@/services";
import { Service } from "@/models";
export default defineComponent({
  components: {
    CTable,
    CTableHead,
    CTableBody,
    CTableRow,
    CTableCell,
    CInput
  },
  data() {
    return {
      loading: false,
      services: [] as Service[],
      isFiltered: false,
      filteredServices: [] as Service[],
      headitems: [
        "name",
        "category",
        "call",
        "min",
        "max",
        "average",
        "total",
        "pending total"
      ]
    };
  },
  created() {
    this.loading = true;
    this.getAll();
    this.loading = false;
  },
  computed: {
    cservices: function(): Service[] {
      if (this.isFiltered) {
        return this.filteredServices;
      }

      return this.services;
    }
  },
  methods: {
    getAll: async function(): Promise<void> {
      this.services = await ApiService.getAllServices();
    },
    filter: function(filter: string): void {
      if (filter.length > 0) {
        this.filteredServices = this.services.filter(service =>
          service.name.includes(filter)
        );
        this.isFiltered = true;
      } else {
        this.isFiltered = false;
      }
    }
  }
});
</script>
