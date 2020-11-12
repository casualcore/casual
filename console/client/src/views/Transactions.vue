<template>
  <div>
    <pre class="float"
      >{{ transaction }}
    </pre>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
import { Transaction } from "@/models";
import { ApiService } from "@/services";
export default defineComponent({
  data() {
    return {
      transaction: {} as Transaction
    };
  },
  async created() {
    await this.getTransactions();
  },
  methods: {
    getTransactions: async function(): Promise<void> {
      this.transaction = await ApiService.getTransactions();
    }
  }
});
</script>
