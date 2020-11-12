<template>
    <button class="button" @click="click"><slot></slot></button>
</template>
<script lang="ts">
import Vue, { defineComponent } from 'vue'
export default defineComponent({
    name: 'CButton',
    methods: {
        click: function(): void {
            this.$emit('clicked');
        }
    }
})
</script>