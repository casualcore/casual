<template>
    <c-table>
        <c-table-head :headitems="headitems"></c-table-head>
        <c-table-body :headitems="headitems">
          <template v-for="server in servers" :key="server.alias">
            <c-table-row>
              <td>
                <router-link
                  class="casual-link"
                  :to="{ name: 'Server', query: { alias: server.getAlias() } }"
                  >{{ server.getAlias() }}</router-link
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
</template>
<script lang="ts">
import Vue, { defineComponent } from 'vue'
export default defineComponent({
    
})
</script>