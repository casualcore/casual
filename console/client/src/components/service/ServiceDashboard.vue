<template>
  <div>
    <div class="row">
      <div class="col">
        <c-card-deck variant="sm">
          <c-value-card :data="service.called">
            <template v-slot:description>Calls</template>
          </c-value-card>
          <c-value-card :data="metric.total">
            <template v-slot:description>Total time</template>
          </c-value-card>
          <!--div class="row">
            <div class="col">
              <c-value-card :data="metric.min" >
                <template v-slot:description>Min</template>
              </c-value-card>
            </div>
            <div class="col">
              <c-value-card :data="metric.max" >
                <template v-slot:description>Max</template>
              </c-value-card>
            </div>
            <div class="col">
              <c-value-card :data="metric.avg" >
                <template v-slot:description>Average</template>
              </c-value-card>
            </div>
            <div class="col">
              <c-value-card :data="metric.total" >
                <template v-slot:description>Total</template>
              </c-value-card>
            </div>
          </div-->
        </c-card-deck>
      </div>
    </div>
    <div class="row">
        <c-card-deck>
          <div class="row">
            <div class="col">
              <c-card>
                <template v-slot:content v-if="service.name">
                  <div class="row" style="text-align: center">
                    <div class="col col-4">
                      <div style="font-size: 7pt;">Min</div>
                      <div style="font-size: 11pt;">{{ metric.min }}</div>
                    </div>
                    <div class="col col-4">
                      <div style="font-size: 7pt;">Average</div>
                      <div style="font-size: 11pt;">{{ metric.avg }}</div>
                    </div>
                    <div class="col col-4">
                      <div style="font-size: 7pt;">Max</div>
                      <div style="font-size: 11pt;">{{ metric.max }}</div>
                    </div>
                  </div>
                </template>
              </c-card>
            </div>
          </div>
          <div class="row">
            <div class="col">
              <c-card>
                <!--template v-slot:cardTop>
              <div class="row p-1" style="text-align: center">
                <div class="col-4">
                  <div style="font-size: 7pt;">Min</div>
                  <div style="font-size: 11pt;">{{ metric.min }}</div>
                </div>
                <div class="col-4">
                  <div style="font-size: 7pt;">Average</div>
                  <div style="font-size: 11pt;">{{ metric.avg }}</div>
                </div>
                <div class="col-4">
                  <div style="font-size: 7pt;">Max</div>
                  <div style="font-size: 11pt;">{{ metric.max }}</div>
                </div>
              </div>
            </template-->
                <!--template v-slot:header><slot name="cardHead"></slot></template-->
                <template v-slot:content v-if="service.name">
                  <bar-chart
                    :c-id="cType"
                    :chart-data="chartMetric"
                    :key="chartMetric"
                  ></bar-chart>
                </template>
              </c-card>
            </div>
          </div>
        </c-card-deck>
      </div>
  </div>
</template>
<script lang="ts">
import { defineComponent } from "vue";
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
      const dataSet = [m.min, m.avg, m.max];
      const labels = ["min", "avg", "max"];
      const color = "#f6a821";

      return { data: dataSet, labels: labels, color: color };
    }
  }
});
</script>