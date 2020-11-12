<template>
  <div>
    <c-card-deck variant="sm">
      <c-value-card :data="metric.min">
        <template v-slot:description>Min</template>
      </c-value-card>
      <c-value-card :data="metric.max">
        <template v-slot:description>Max</template>
      </c-value-card>
      <c-value-card :data="metric.avg">
        <template v-slot:description>Average</template>
      </c-value-card>
      <c-value-card :data="service.called">
        <template v-slot:description>Calls</template>
      </c-value-card>
      <c-value-card :data="metric.total">
        <template v-slot:description>Total</template>
      </c-value-card>
    </c-card-deck>
    <c-card-deck>
      <c-card>
        <template v-slot:header><slot name="cardHead"></slot></template>
        <template v-slot:content v-if="service.name">
          <bar-chart
            :c-id="cType"
            :chart-data="chartMetric"
            :key="service.name"
          ></bar-chart>
        </template>
      </c-card>
    </c-card-deck>
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
import { CCardDeck, CValueCard, CCard } from "@/components/card";
import { BarChart } from "@/components/chart";
export default defineComponent({
  name: "ServiceDashboard",
  props: ["service", "metric", "cType"],
  components: {
    CCardDeck,
    CValueCard,
    CCard,
    BarChart
  },
  computed: {
    chartMetric: function(): object {
      const m = this.metric;
      const dataSet = [m.min, m.max, m.avg];
      const labels = ["min", "max", "avg"];
      const color = "#f6a821";

      return { data: dataSet, labels: labels, color:color };
    }
  }
});
</script>