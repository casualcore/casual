<template>
  <canvas :id="cId"></canvas>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
import Chart from "chart.js";
export default defineComponent({
  name: "BarChart",
  props: ["cId", "chartData"],
  data() {
    return {
      options: {
        scales: {
          yAxes: [
            {
              ticks: {
                beginAtZero: true,
                fontColor: "#24262d"
              }
            }
          ],
          xAxes: [
            {
              ticks: {
                fontColor: "#24262d"

              }
            }
          ]
        },
        legend: {
          display: false
        }
      }
    };
  },
  mounted() {
    this.createChart({
      datasets: [
        {
          label: "sec",
          data: this.chartData.data,
          backgroundColor: this.chartData.color
        }
      ],
      labels: this.chartData.labels
    });
  },
  methods: {
    createChart: function(chartData: object): void {
      const canvas = document.getElementById(this.cId) as HTMLCanvasElement;
      const options = {
        type: "bar",
        data: chartData,
        options: this.options
      };
      new Chart(canvas, options);
    }
  }
});
</script>