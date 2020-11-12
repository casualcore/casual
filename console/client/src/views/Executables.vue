<template>
  <div>
    <div class="float">
      <c-table>
        <c-table-head :headitems="headitems"></c-table-head>
        <c-table-body>
          <template v-for="executable in executables" :key="executable.alias">
            <c-table-row>
              <td>
                <router-link
                  class="casual-link"
                  :to="{
                    name: 'Executable',
                    query: { alias: executable.alias }
                  }"
                  >{{ executable.alias }}</router-link
                >
              </td>
              <td>
                <span>{{ executable.note }}</span>
              </td>
              <td>
                <router-link
                  class="casual-link"
                  v-for="group in executable.memberof"
                  :key="group"
                  :to="{ name: 'Group', query: { name: group } }"
                  >{{ group }}</router-link
                >
              </td>
            </c-table-row>
          </template>
        </c-table-body>
      </c-table>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import { Server } from "@/models";
import { ApiService } from "@/services";
import { CTable, CTableHead, CTableBody, CTableRow } from "@/components/table";
export default defineComponent({
  name: "Servers",
  components: {
    CTable,
    CTableHead,
    CTableBody,
    CTableRow
  },
  data() {
    return {
      executables: [] as Server[],
      headitems: ["Alias", "Note", "Memberof"]
    };
  },
  async created() {
    await this.getAll();
  },
  methods: {
    getAll: async function(): Promise<void> {
      this.executables = await ApiService.getAllExecutables();
    }
  }
});
</script>
