<template>
  <div>
    <div class="float">
      <div class="p-1">
        <div class="big">Groups</div>
        
        <c-input
          class="right"
          placeholder="Filter"
          @changeInput="filter"
        ></c-input>
      </div>
      <c-table :key="filteredGroups">
        <c-table-head :headitems="headitems" />
        <c-table-body>
          <c-table-row v-for="group in cgroups" :key="group.name">
            <c-table-cell route="Group" :query="{ name: group.name }">{{
              group.name
            }}</c-table-cell>
            <c-table-cell>{{ group.note }}</c-table-cell>
            <c-table-cell>{{ group.members.length }}</c-table-cell>
          </c-table-row>
        </c-table-body>
      </c-table>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import { ApiService } from "@/services";
import { Group } from "@/models";
import { CInput, CButton, CButtonGroup, CSelectNbrItems } from "@/components/input";
import {
  CTable,
  CTableHead,
  CTableBody,
  CTableRow,
  CTableCell
} from "@/components/table";
export default defineComponent({
  name: "Groups",
  components: {
    CTable,
    CTableHead,
    CTableBody,
    CTableRow,
    CTableCell,
    CInput,
  },
  data() {
    return {
      groups: [] as Group[],
      headitems: ["name", "note", "members"],
      filteredGroups: [] as Group[],
      isFiltered: false,
      page: 1
    };
  },
  created(): void {
    this.getAll();
  },
  computed: {
    cgroups: function(): Group[] {
      if (this.isFiltered) {
        return this.filteredGroups;
      }
      return this.groups;
    }
  },
  methods: {
    getAll: async function(): Promise<void> {
      this.groups = await ApiService.getAllGroups();
    },
    filter: function(filter: string): void {
      if (filter.length > 0) {
        this.filteredGroups = this.groups.filter(group =>
          group.name.includes(filter)
        );
        this.isFiltered = true;
      } else {
        this.isFiltered = false;
      }
    },
    nbrItems: function(nbr: number) {
        console.log(nbr);
    }
  }
});
</script>
