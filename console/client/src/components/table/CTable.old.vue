<template>
  <table class="casual-table">
    <thead>
      <tr class="casual-table-head">
        <th v-for="k in keys" :key="k">{{ k }}</th>
      </tr>
    </thead>
    <tbody>
      <tr v-for="r in values" :key="r" class="casual-table-item">
        <td v-for="(v, index) in r" :key="index">
          <template v-if="cellItemIsArray(v)">
            <ul>
              <li v-for="(it, index) in getCellItems(v)" :key="index">
                <template v-if="it.isLink">
                  <router-link :to="{ name: itemConfig.route }">{{
                    it
                  }}</router-link>
                </template>
                <template v-else>{{ it }}</template>
              </li>
            </ul>
          </template>
          <template v-else>{{ v }}</template>
        </td>
      </tr>
    </tbody>
  </table>
</template>
<script lang="ts">
import Vue, { defineComponent, PropType } from "vue";
interface TableItem {
  [key: number]: string;
  isLink: boolean;
}
interface ItemConfig {
  key: string;
  route: string;
  query: string;
}

export default defineComponent({
  name: "CTable",
  props: {
    items: {
      type: Array as PropType<Array<TableItem>>,
      default: []
    },
    itemConfig: {
      type: Object as PropType<ItemConfig>,
      default: {}
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
    getRowValue: function(item: TableItem): any[] {
      const values: string[] = Object.values(item);

      return values;
    },
    cellItemIsArray(item: TableItem): boolean {
      return Array.isArray(item);
    },
    getCellItems: function(item: TableItem): TableItem | string[] {
      if (Array.isArray(item)) {
        console.log("IS ARRAY");
        const items: string[] = [];
        for (const it of this.getRowValue(item)) {
          //const it: Record<string, any> = this.getRowValue(item);
          const key: string = Object.keys(it)[0];
          //console.log(it[key]);
          if (this.isLink(key)) {
            it.isLink = true;
          }
          items.push(it[key]);
        }
        return items;
      }
      console.log(item);
      return item;
    },
    isLink: function(key: string): boolean {
      return this.itemConfig.key === key;
    }
  }
});
</script>
