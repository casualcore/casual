<template>
  <div>
    <div class="row mb-2 ">
      <div class="col ">
        <div class="big ">{{ service.name }}</div>
      </div>
      <div class="col ">
        <c-button class="button" @clicked="resetMetrics"
          >reset metrics</c-button
        >
      </div>
    </div>
    <div class="row">
      <div class="col">
        <c-card variant="" class="ptb-1 mb-1">
            <template v-slot:header>Summary</template>
          <template v-slot:content>
            <div class="row">
              <div class="col">Category</div>
              <div class="col right">
                <b>{{ service.category }}</b>
              </div>
            </div>
            <div class="row">
              <div class="col ">Last called</div>
              <div class="col right">
                <b>{{ lastCalled }}</b>
              </div>
            </div>
            <div class="row">
              <div class="col " >Parent Server</div>
              <div class="col right">
                <router-link class="casual-link"
                  :to="{ name: 'Server', query: { alias: service.parent } }"
                  >{{ service.parent }}</router-link
                >
              </div>
            </div>
          </template>
        </c-card>
      </div>
      {{ service.metric }}
    </div>
    <div class="row">
      <div class="col ">
        <div class="big">
          Invoked
        </div>
        <c-card-deck>
          <service-dashboard
            :service="service"
            :metric="service.invoked"
            cType="invoked"
          >
            <template v-slot:cardHead>Invoked</template>
          </service-dashboard>
        </c-card-deck>
      </div>
      <div class="col ">
        <div class="big">
          Pending
        </div>
        <c-card-deck>
          <service-dashboard
            :service="service"
            :metric="service.pending"
            cType="pending"
          >
            <template v-slot:cardHead>Pending</template>
          </service-dashboard>
        </c-card-deck>
      </div>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import { CCardDeck, CValueCard, CCard } from "@/components/card";
import { ServiceExtended } from "@/models";
import { ApiService, AdminService } from "@/services";
import { ServiceDashboard } from "@/components/service";
import { BarChart } from "@/components/chart";
import { CButton } from "@/components/input";
export default defineComponent({
  name: "Server",
  components: {
    ServiceDashboard,
    CCardDeck,
    CButton,
    CCard
  },
  data() {
    return {
      name: this.$route.query.name as string,
      service: new ServiceExtended(),
      tabs: ["summary"]
    };
  },
  async created() {
    await this.getService();
  },
  computed: {
    lastCalled: function(): string {
      let last = "";
      if (this.service.last) {
        last = this.service.last;
      }

      return last;
    }
  },
  methods: {
    getService: async function(): Promise<void> {
      try {
        this.service = await ApiService.getServiceByName(this.name);
      } catch (error) {
        console.log(error);
      }
    },
    resetMetrics: async function(): Promise<void> {
      try {
        const reset = await AdminService.resetMetrics([this.service.name]);
        if (reset.includes(this.service.name)) {
          await this.getService();
        }
      } catch (error) {
        console.log(error);
      }
    }
  }
});
</script>
