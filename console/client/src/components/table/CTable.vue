<template>
  <table class="casual-table">
    <thead>
      <tr class="casual-table-head">
        <th v-for="k in keys" :key="k">{{ k }}</th>
      </tr>
    </thead>
    <tbody>
      <tr v-for="r in values" :key="r" class="casual-table-item">
        <td v-for="(v, index) in r" :key="index">{{ v }}</td>
      </tr>
    </tbody>
  </table>
</template>
<script lang="ts">
import Vue, { defineComponent, PropType } from "vue";
interface TableItem {
  [key: string]: string;
}
export default defineComponent({
  name: "CTable",
  props: {
    items: {
      type: Array as PropType<Array<TableItem>>,
      default: []
    }
  },
  data() {
    return {
      keys: [] as string[],
      values: [] as string[][]
    };
  },
  created() {
    this.keys = this.getKeys;
    this.values = this.getValues();
  },
  computed: {
    rows: function(): number {
      return this.items.length;
    },
    columns: function(): number {
      let cols = 0;
      if (this.rows > 0) {
        cols = this.keys.length;
      }
      return cols;
    },
    getKeys: function(): string[] {
      let itemKeys = [] as string[];
      if (this.rows > 0) {
        itemKeys = Object.keys(this.items[0]);
      }
      return itemKeys;
    }
  },
  methods: {
    getValues: function(): string[][] {
      const values = [];

      const items: TableItem[] = this.items;

      for (let i = 0; i < items.length; i++) {
        values[i] = this.getRowValue(items[i]);
      }

      return values;
    },
    getRowValue: function(item: TableItem): string[] {
      const values: string[] = Object.values(item);
      return values;
    }
  }
});
</script>
