<template>
  <div :key="server.alias">
    <div class="">
      <div>
        <div class="big">{{ server.alias }}</div>
        <button style="float: right;" class="button ">Restart</button>
      </div>
    </div>
    <c-card-deck variant="sm">
      <c-value-card :data="running">
        <template v-slot:description>Running</template>
      </c-value-card>

      <c-value-card :data="server.restarts">
        <template v-slot:description>Restarts</template>
      </c-value-card>

      <c-value-card :data="server.restart">
        <template v-slot:description>Restart</template>
      </c-value-card>
    </c-card-deck>
    <c-card-deck>
      <c-card>
        <template v-slot:header>Member of</template>
        <template v-slot:content>
          <div v-for="member in server.memberof" :key="member">
            <router-link
              class="casual-link"
              :to="{ name: 'Group', query: { name: member } }"
            >
              {{ member }}
            </router-link>
          </div>
        </template>
      </c-card>
    </c-card-deck>
  </div>
</template>
<script lang="ts">
import { ServerExtended } from "@/models/server";
import { ApiService } from "@/services";
import Vue, { defineComponent } from "vue";
import { CCard, CCardDeck, CValueCard } from "@/components/card";
import { LocationQueryValue } from "vue-router";

export default defineComponent({
  name: "Server",
  components: {
    CCardDeck,
    CValueCard,
    CCard
  },
  data() {
    return {
      alias: this.$route.query.alias as string,
      server: new ServerExtended()
    };
  },
  async created() {
    await this.getServer();
  },
  computed: {
    running: function(): number {
      if (this.server.instancesCount) {
        return this.server.instancesCount.running.length;
      } else return 0;
    }
  },
  methods: {
    getServer: async function(): Promise<void> {
      this.server = await ApiService.getServerByAlias(this.alias);
    }
  }
});
</script>
