<template>
  <div>
    <pre class="card"
      >{{ gateway }}
    </pre>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import { Gateway } from "@/models";
import { ApiService } from "@/services";
export default defineComponent({
  data() {
    return {
      gateway: {} as Gateway
    };
  },
  async created() {
    await this.getGateway();
  },
  methods: {
    getGateway: async function(): Promise<void> {
      this.gateway = await ApiService.getGateways();
    }
  }
});
</script>
