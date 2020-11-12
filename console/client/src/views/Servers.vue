<template>
  <div>
    <div class="float">
      <div class="p-1">
        <div class="big">Servers</div>
        <c-input class="right" placeholder="Filter"></c-input>
      </div>
      <c-table>
        <c-table-head :headitems="headitems"></c-table-head>
        <c-table-body :headitems="headitems">
          <template v-for="server in servers" :key="server.alias">
            <c-table-row>
              <td>
                <router-link
                  class="casual-link"
                  :to="{ name: 'Server', query: { alias: server.alias } }"
                  >{{ server.alias }}</router-link
                >
              </td>
              <td>
                <span>{{ server.note }}</span>
              </td>
              <td>
                <router-link
                  class="casual-link"
                  v-for="group in server.memberof"
                  :key="group.name"
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
import { CInput } from "@/components/input";
export default defineComponent({
  name: "Servers",
  components: {
    CTable,
    CTableHead,
    CTableBody,
    CTableRow,
    CInput
  },
  data() {
    return {
      servers: [] as Server[],
      headitems: ["Alias", "Note", "Memberof"]
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
