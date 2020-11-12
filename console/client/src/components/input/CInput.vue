<template>
  <div class="casual-input">
    <slot name="label" class="casual-label"></slot>
    <div class="input-control">
      <input type="text" :placeholder="placeholder" v-model="value">
      <button class="clear-button" v-if="value" @click="clear">&times;</button>
    </div>
  </div>
</template>
<script lang="ts">
import Vue, { defineComponent } from "vue";
export default defineComponent({
    name: 'CInput',
    props: ['placeholder'],
    data() {
        return {
            value: ""
        }
    },
    methods: {
        input: function(): void {
            this.$emit('input', this.value);
        },
        clear: function(): void {
            this.value = "";
            this.input();
        }
    },
    watch: {
        value: function(newVal): void{
            this.$emit('change-input', newVal);
        }
    }
});
</script>