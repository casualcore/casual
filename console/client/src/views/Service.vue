<template>
  <div>
      <div class="row">
        <div class="big ptb-1" >{{ service.name }}</div>
      </div>
    <c-button class="button" @clicked="resetMetrics">reset metrics</c-button>
    <c-card-deck>
      <service-dashboard
        :service="service"
        :metric="service.invoked"
        cType="invoked"
      >
        <template v-slot:cardHead>Invoked</template>
      </service-dashboard>
    </c-card-deck>
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
    CButton
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
